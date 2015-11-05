#include <fiberize/detail/executor.hpp>

#include <thread>
#include <chrono>

namespace fiberize {
namespace detail {

Executor::Executor(): runQueue(100) {
    thread = std::thread(&Executor::run, this);
}

void Executor::execute(ControlBlock* controlBlock) {
    runQueue.push(controlBlock);
}

ControlBlock* Executor::currentControlBlock() {
    return currentControlBlock_;
}

void Executor::suspend() {
    /**
     * Context switch back to the control loop.
     */
    boost::context::jump_fcontext(&currentControlBlock_->context, returnContext, 0);
}

void Executor::run() {
    /**
     * Set the thread local executor to this.
     */
    Executor::current = this;
    
    /**
     * Loop forever executing fibers.
     */
    ControlBlock* controlBlock;
    while (true) {
        /**
         * Busy-wait until we have something to do.
         * TODO: do not busy wait
         */
        while (!runQueue.pop(controlBlock)) {
            // TODO: change this to conditions.
            using namespace std::literals;
            std::this_thread::sleep_for(1ms);
        }
   
        /**
         * Context switch to a fiber.
         */
        currentControlBlock_ = controlBlock;
        boost::context::jump_fcontext(&returnContext, controlBlock->context, 0);
        currentControlBlock_ = nullptr;
        
        /**
         * If the fiber didn't finish put it back on the queue.
         */
        if (!controlBlock->finished) {
            runQueue.push(controlBlock);
        } else {
            /**
             * Otherwise destroy the control block.
             */
            // TODO: free memory
        }
    }
}

thread_local Executor* Executor::current = nullptr;

} // namespace detail    
} // namespace fiberize
