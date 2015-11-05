#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/system.hpp>

#include <thread>
#include <chrono>

namespace fiberize {
namespace detail {

Executor::Executor(fiberize::System* system): runQueue(100), system_(system) {
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

System* Executor::system() {
    return system_;
}

Executor* Executor::current() {
    return current_;
}

void Executor::run() {
    /**
     * Set the thread local executor to this.
     */
    Executor::current_ = this;
    
    /**
     * Loop forever executing fibers.
     */
    ControlBlock* controlBlock;
    while (true) {
        /**
         * Busy-wait until we have something to do.
         * TODO: work stealing and epoll
         */
        while (!runQueue.pop(controlBlock)) {
            // TODO: change this to conditions or sth
            using namespace std::literals;
            std::this_thread::sleep_for(1ns);
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
        if (!controlBlock->exited) {
            runQueue.push(controlBlock);
        } else {
            /**
             * Otherwise destroy the control block.
             */
            delete controlBlock->fiber;
            controlBlock->mailbox->drop();
            system()->stackAllocator.deallocate(controlBlock->stack);
            delete controlBlock;
        }
    }
}

thread_local Executor* Executor::current_ = nullptr;

} // namespace detail    
} // namespace fiberize
