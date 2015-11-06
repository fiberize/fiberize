#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/system.hpp>

#include <thread>
#include <chrono>

namespace fiberize {
namespace detail {

Executor::Executor(fiberize::System* system, uint64_t seed, uint32_t myIndex)
    : runQueue(1024)
    , system(system)
    , randomEngine(seed)
    , myIndex(myIndex)
    , stackPool(new detail::CachedFixedSizeStackPool)
    , currentControlBlock_(nullptr)
    , previousControlBlock_(nullptr)
    , emergencyStop(false)
    {}

void Executor::start() {
    thread = std::thread(&Executor::idle, this);    
}

void Executor::stop() {
    emergencyStop = true;
    thread.join();
}

void Executor::schedule(ControlBlock* controlBlock) {
    assert(controlBlock->status == detail::Suspended);
    controlBlock->status = detail::Scheduled;
    controlBlock->mutex.unlock();
    controlBlock->grab();
    runQueue.push(controlBlock);
}

ControlBlock* Executor::currentControlBlock() {
    return currentControlBlock_;
}

void Executor::suspend() {    
    /**
     * Switch to the next fiber.
     */
    switchFromRunning();
}

void Executor::terminate() {
    /**
     * Destroy the control block.
     */
    currentControlBlock_->status = detail::Dead;
    currentControlBlock_->mutex.unlock();
    stackPool->delayedDeallocate(currentControlBlock_->stack);
    currentControlBlock_->drop();
    currentControlBlock_ = nullptr;
    
    /**
     * Switch to the next fiber.
     */
    switchFromTerminated();
}

Executor* Executor::current() {
    return current_;
}

void Executor::idle() {
    /**
     * Set the thread local executor to this.
     */
    Executor::current_ = this;
    
    /**
     * Idle loop.
     */
    ControlBlock* controlBlock;
    std::uniform_int_distribution<uint32_t> randomExecutor(0, system->executors.size() - 2);

    while (!emergencyStop) {
        while (runQueue.pop(controlBlock)) {
            jumpToFiber(&idleContext, controlBlock);
            afterJump();
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
            if (system->executors[i]->runQueue.pop(controlBlock)) {
                jumpToFiber(&idleContext, controlBlock);
                afterJump();
            } else {
                using namespace std::literals;
                std::this_thread::sleep_for(1ns);
            }
        }
    }
}

void Executor::switchFromRunning() {
    ControlBlock* controlBlock;
    
    if (runQueue.pop(controlBlock)) {
        /**
         * Switch the current control block to the next fiber and make the jump
         * saving our current state to the control block.
         */
        jumpToFiber(&currentControlBlock_->context, controlBlock);
            
        /**
         * We returned from the jump, this means another fiber switched to us. 
         * The current control block was restored by the fiber making the jump.
         */
        afterJump();
    } else {
        /**
         * Jump to the idle context.
         */
        jumpToIdle(&currentControlBlock_->context);
    }
}

void Executor::switchFromTerminated() {
    ControlBlock* controlBlock;
 
    if (runQueue.pop(controlBlock)) {
        /**
         * Switch the current control block to the next fiber and make the jump
         * saving our current state to the control block.
         */
        jumpToFiber(&dummyContext, controlBlock);
            
        /**
          * The jump cannot return.
          */
        __builtin_unreachable();
    } else {
        /**
         * Jump to the idle context.
         */
        jumpToIdle(&dummyContext);
    }
}

void Executor::jumpToIdle(boost::context::fcontext_t* stash) {
    previousControlBlock_ = currentControlBlock_;
    currentControlBlock_ = nullptr;
    
    boost::context::jump_fcontext(stash, idleContext, 0);    
}

void Executor::jumpToFiber(boost::context::fcontext_t* stash, ControlBlock* controlBlock) {
    previousControlBlock_ = currentControlBlock_;
    currentControlBlock_ = controlBlock;

    if (controlBlock->stack.sp == nullptr) {
        controlBlock->stack = stackPool->allocate();
        controlBlock->context = boost::context::make_fcontext(controlBlock->stack.sp, controlBlock->stack.size, &Executor::fiberRunner);
    }

    boost::context::jump_fcontext(stash, controlBlock->context, 0);
}

void Executor::afterJump() {
    if (previousControlBlock_ != nullptr) {
        /**
         * We jumped from a fiber and are holding the mutex on it. 
         * Suspend that fiber and drop the reference.
         */
        previousControlBlock_->status = detail::Suspended;
        previousControlBlock_->mutex.unlock();
        previousControlBlock_->drop();
        previousControlBlock_ = nullptr;
    }
    
    if (currentControlBlock_ != nullptr) {
        /**
         * We jumped to a fiber. Set its status to running.
         */
        currentControlBlock_->mutex.lock();
        currentControlBlock_->status = detail::Running;
        currentControlBlock_->mutex.unlock();
    }
}

void Executor::fiberRunner(intptr_t) {
    /**
     * Change the status to Running.
     */
    detail::Executor::current()->afterJump();
    
    /**
     * Setup the context.
     */
    auto controlBlock = detail::Executor::current()->currentControlBlock();
    auto system = detail::Executor::current()->system;
    Context context(controlBlock, system);

    /**
     * Execute the fiber.
     */
    controlBlock->fiber->_execute();
    
    /**
     * Terminate the fiber.
     */
    system->fiberFinished();
    controlBlock->mutex.lock();
    detail::Executor::current()->terminate();
}

thread_local Executor* Executor::current_ = nullptr;

} // namespace detail    
} // namespace fiberize
