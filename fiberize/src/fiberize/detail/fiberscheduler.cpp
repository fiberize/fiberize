#include <fiberize/detail/fiberscheduler.hpp>
#include <fiberize/detail/stackpool.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/runnable.hpp>

#include <thread>
#include <chrono>

using namespace std::literals;

namespace fiberize {
namespace detail {

FiberScheduler::FiberScheduler(fiberize::FiberSystem* system, uint64_t seed, uint32_t index)
    : Scheduler(system, seed)
    , runQueue(1024)
    , previousControlBlock_(nullptr)
    , currentControlBlock_(nullptr)
    , myIndex(index)
    , stackPool(new detail::CachedFixedSizeStackPool)
    , emergencyStop(false)
    {}

FiberScheduler::~FiberScheduler() {}

void FiberScheduler::start() {
    thread = std::thread(&FiberScheduler::idle, this);
}

void FiberScheduler::stop() {
    emergencyStop = true;
    thread.join();
}

void FiberScheduler::enableFiber(FiberControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    controlBlock->status = detail::Scheduled;
    lock.unlock();
    runQueue.enqueue(controlBlock);
}

void FiberScheduler::suspend(boost::unique_lock<ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    currentControlBlock_->reschedule = false;

    /**
     * Switch to the next fiber.
     */
    switchFromRunning(std::move(lock));
}

void FiberScheduler::yield(boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    currentControlBlock_->reschedule = true;

    /**
     * Switch to the next fiber.
     */
    switchFromRunning(std::move(lock));
}

void FiberScheduler::terminate() {
    /**
     * Destroy the control block.
     */
    currentControlBlock_->status = detail::Dead;
    stackPool->delayedDeallocate(currentControlBlock_->stack);
    currentControlBlock_->runnable.reset();
    currentControlBlock_->drop();
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

ControlBlock* FiberScheduler::currentControlBlock() {
    return currentControlBlock_;
}

void FiberScheduler::idle() {
    makeCurrent();

    /**
     * Idle loop.
     */
    FiberControlBlock* controlBlock;
    std::uniform_int_distribution<uint32_t> randomFiberScheduler(0, system()->schedulers().size() - 2);

    while (!emergencyStop) {
        while (runQueue.try_dequeue(controlBlock)) {
            assert(controlBlock->status == Scheduled);
            jumpToFiber(&idleContext, std::move(controlBlock));
        }

        if (system()->schedulers().size() > 1) {
            /**
             * Choose a random executor that is not ourself.
             */
            uint32_t i = randomFiberScheduler(random());
            if (i >= myIndex)
                i += 1;

            /**
             * Try to take his job.
             */
            if (system()->schedulers()[i]->runQueue.try_dequeue(controlBlock)) {
                assert(controlBlock->status == Scheduled);
                jumpToFiber(&idleContext, std::move(controlBlock));
            } else {
                std::this_thread::sleep_for(1ns);
            }
        } else {
            std::this_thread::sleep_for(1ns);
        }
    }
}

void FiberScheduler::switchFromRunning(boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    assert(lock.owns_lock());
    FiberControlBlock* controlBlock;
    assert(EventContext::current() != nullptr);

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

    assert(EventContext::current() != nullptr);
}

Void FiberScheduler::switchFromTerminated() {
    FiberControlBlock* controlBlock;

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

void FiberScheduler::jumpToIdle(boost::context::fcontext_t* stash) {
    previousControlBlock_ = currentControlBlock_;
    currentControlBlock_ = nullptr;

    boost::context::jump_fcontext(stash, idleContext, 0);
    static_cast<FiberScheduler*>(current())->afterJump();
}

void FiberScheduler::jumpToFiber(boost::context::fcontext_t* stash, FiberControlBlock* controlBlock) {
    previousControlBlock_ = currentControlBlock_;
    currentControlBlock_ = controlBlock;

    if (previousControlBlock_ != nullptr) {
        assert(previousControlBlockLock.owns_lock());
    }

    if (currentControlBlock_->stack.sp == nullptr) {
        currentControlBlock_->stack = stackPool->allocate();
        currentControlBlock_->context = boost::context::make_fcontext(currentControlBlock_->stack.sp, currentControlBlock_->stack.size, &FiberScheduler::fiberRunner);
    }

    boost::context::jump_fcontext(stash, currentControlBlock_->context, reinterpret_cast<intptr_t>(this));
    static_cast<FiberScheduler*>(current())->afterJump();
}

void FiberScheduler::afterJump() {
    if (previousControlBlock_ != nullptr) {
        /**
         * We jumped from a fiber and are holding the mutex on it.
         * Suspend that fiber and drop the reference.
         */
        assert(previousControlBlockLock.owns_lock());
        previousControlBlock_->status = Suspended;

        if (previousControlBlock_->reschedule) {
            enableFiber(previousControlBlock_, std::move(previousControlBlockLock));
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
        currentControlBlock_->eventContext->makeCurrent();
    }
}

void FiberScheduler::fiberRunner(intptr_t) {
    /**
      * Change the status to Running.
      */
    static_cast<FiberScheduler*>(current())->afterJump();

    /**
     * We need a block here to make sure destructors are executed,
     * because terminate() doesn't return.
     */
    {
        auto controlBlock = current()->currentControlBlock();

        /**
         * Prepare the event context.
         */
        EventContext ectx(current()->system(), current()->currentControlBlock());
        controlBlock->eventContext = &ectx;
        ectx.makeCurrent();

        /**
         * Execute the fiber.
         */
        static_cast<FiberScheduler*>(current())->currentControlBlock_->runnable->operator()();
    }

    /**
     * Terminate the fiber.
     */
    current()->system()->fiberFinished();
    current()->terminate();
}

} // namespace detail
} // namespace fiberize
