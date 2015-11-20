#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/fibersystem.hpp>

#include <thread>
#include <chrono>

namespace fiberize {
namespace detail {

Executor::Executor(fiberize::FiberSystem* system, uint64_t seed, uint32_t myIndex)
    : system(system)
    , runQueue(1024)
    , previousControlBlock_(nullptr)
    , currentControlBlock_(nullptr)
    , randomEngine(seed)
    , myIndex(myIndex)
    , stackPool(new detail::CachedFixedSizeStackPool)
    , emergencyStop(false)
    {}

void Executor::start() {
    thread = std::thread(&Executor::idle, this);
}

void Executor::stop() {
    emergencyStop = true;
    thread.join();
}

void Executor::schedule(detail::ControlBlockPtr controlBlock, boost::unique_lock< fiberize::detail::ControlBlockMutex >&& lock) {
    assert(lock.owns_lock());
    assert(controlBlock->status == detail::Suspended);
    controlBlock->status = detail::Scheduled;
    lock.unlock();
    runQueue.enqueue(std::move(controlBlock));
}

ControlBlockPtr Executor::currentControlBlock() {
    return currentControlBlock_;
}

void Executor::suspend(boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    currentControlBlock_->reschedule = false;

    /**
     * Switch to the next fiber.
     */
    switchFromRunning(std::move(lock));
}

void Executor::suspendAndReschedule(boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    currentControlBlock_->reschedule = true;

    /**
     * Switch to the next fiber.
     */
    switchFromRunning(std::move(lock));
}

Void Executor::terminate() {
    /**
     * Destroy the control block.
     */
    currentControlBlock_->status = detail::Dead;
    stackPool->delayedDeallocate(currentControlBlock_->stack);
    currentControlBlock_->fiber.reset();
    currentControlBlock_ = nullptr;

    /**
     * Switch to the next fiber.
     */
    switchFromTerminated();

    /**
     * The jump doesn't return.
     */
    __builtin_unreachable();
}

void Executor::idle() {
    current_ = this;

    /**
     * Idle loop.
     */
    ControlBlockPtr controlBlock;
    std::uniform_int_distribution<uint32_t> randomExecutor(0, system->executors.size() - 2);

    while (!emergencyStop) {
        while (runQueue.try_dequeue(controlBlock)) {
            assert(controlBlock->status == Scheduled);
            jumpToFiber(&idleContext, std::move(controlBlock));
        }

        if (system->executors.size() > 1) {
            /**
            * Choose a random executor that is not ourself.
            */
            uint32_t i = randomExecutor(randomEngine);
            if (i >= myIndex)
                i += 1;

            /**
            * Try to take his job.
            */
            if (system->executors[i]->runQueue.try_dequeue(controlBlock)) {
                assert(controlBlock->status == Scheduled);
                jumpToFiber(&idleContext, std::move(controlBlock));
            } else {
                // TODO: conditions?
                pthread_yield();
            }
        } else {
            pthread_yield();
        }
    }
}

void Executor::switchFromRunning(boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    ControlBlockPtr controlBlock;
    assert(FiberContext::current() != nullptr);

    if (runQueue.try_dequeue(controlBlock)) {
        assert(controlBlock->status == Scheduled);

        /**
         * Make sure we don't context switch into the same context.
         */
        if (controlBlock == currentControlBlock_) {
            controlBlock->status = Running;
            return;
        } else {
            previousControlBlockLock = std::move(lock);

            /**
             * Switch the current control block to the next fiber and make the jump
             * saving our current state to the control block.
             */
            jumpToFiber(&currentControlBlock_->context, std::move(controlBlock));
        }
    } else {
        previousControlBlockLock = std::move(lock);

        /**
         * Jump to the idle context.
         */
        jumpToIdle(&currentControlBlock_->context);
    }

    assert(FiberContext::current() != nullptr);
}

Void Executor::switchFromTerminated() {
    ControlBlockPtr controlBlock;

    if (runQueue.try_dequeue(controlBlock)) {
        assert(controlBlock->status == Scheduled);

        /**
         * Switch the current control block to the next fiber and make the jump
         * saving our current state to the control block.
         */
        jumpToFiber(&dummyContext, std::move(controlBlock));

    } else {
        /**
         * Jump to the idle context.
         */
        jumpToIdle(&dummyContext);
    }

    /**
     * Both jumps cannot return.
     */
    __builtin_unreachable();
}

void Executor::jumpToIdle(boost::context::fcontext_t* stash) {
    previousControlBlock_ = currentControlBlock_;
    currentControlBlock_ = nullptr;

    boost::context::jump_fcontext(stash, idleContext, 0);
    current_->afterJump();
}

void Executor::jumpToFiber(boost::context::fcontext_t* stash, ControlBlockPtr&& controlBlock) {
    previousControlBlock_ = std::move(currentControlBlock_);
    currentControlBlock_ = std::move(controlBlock);

    if (previousControlBlock_ != nullptr) {
        assert(previousControlBlockLock.owns_lock());
    }

    if (currentControlBlock_->stack.sp == nullptr) {
        currentControlBlock_->stack = stackPool->allocate();
        currentControlBlock_->context = boost::context::make_fcontext(currentControlBlock_->stack.sp, currentControlBlock_->stack.size, &Executor::fiberRunner);
    }

    boost::context::jump_fcontext(stash, currentControlBlock_->context, reinterpret_cast<intptr_t>(this));
    current_->afterJump();
}

void Executor::afterJump() {
    if (previousControlBlock_ != nullptr) {
        /**
         * We jumped from a fiber and are holding the mutex on it.
         * Suspend that fiber and drop the reference.
         */
        assert(previousControlBlockLock.owns_lock());
        previousControlBlock_->status = Suspended;

        if (previousControlBlock_->reschedule) {
            system->schedule(previousControlBlock_, std::move(previousControlBlockLock));
        } else {
            previousControlBlockLock.unlock();
        }

        assert(!previousControlBlockLock.owns_lock());
        previousControlBlock_ = nullptr;
    } else {
        assert(!previousControlBlockLock.owns_lock());
    }

    if (currentControlBlock_ != nullptr) {
        /**
         * We jumped to a fiber. Set its status to running. We can get away with not locking
         * the mutex here, because the whole locking mechanism is only used to ensure two things:
         *   - that only one thread reschedules a suspended thread,
         *   - that we only reschedule a thread after we stop using its stack.
         * After the thread is scheduled, we can change its status without locking.
         */
        assert(currentControlBlock_->status == detail::Scheduled);
        currentControlBlock_->status = detail::Running;
        currentControlBlock_->fiberContext->makeCurrent();
    }
}

void Executor::fiberRunner(intptr_t) {
    /**
      * We grab the control block from the current executor. Note that the executor can change every
      * time we suspend the fiber, due to work stealing.
      */
    ControlBlockPtr controlBlock = current()->currentControlBlock_;

    /**
      * Change the status to Running.
      */
    current()->afterJump();

    /**
      * Execute the fiber.
      */
    controlBlock->fiber->_execute(std::move(controlBlock));

    /**
     * Terminate the fiber.
     */
    current()->system->fiberFinished();
    current()->terminate();
}

Executor* Executor::current() {
    return current_;
}

thread_local Executor* Executor::current_ = nullptr;

} // namespace detail
} // namespace fiberize
