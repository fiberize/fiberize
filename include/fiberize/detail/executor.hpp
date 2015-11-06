#ifndef FIBERIZE_DETAIL_EXECUTOR_HPP
#define FIBERIZE_DETAIL_EXECUTOR_HPP

#include <thread>

#include <boost/lockfree/queue.hpp>
#include <boost/context/all.hpp>

#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/stackpool.hpp>

namespace fiberize {

class System;    

namespace detail {
    
class Executor {
public:
    /**
     * Creates a new executor.
     */
    Executor(System* system, uint64_t seed, uint32_t myIndex);
    
    /**
     * Starts the executor.
     */
    void start();
    
    /**
     * Stops the executor in a hacky way.
     */
    void stop();
    
    //// Fiber lifecycle.
    
    /**
     * Schedules a fiber to be executed by this executor.
     * 
     * You must hold the control block mutex.
     */
    void schedule(ControlBlock* controlBlock);
    
    /**
     * Suspend this fiber.
     * 
     * You must hold the control block mutex.
     */
    void suspend();
    
    /**
     * Terminate this fiber.
     * 
     * You must hold the control block mutex.
     */
    void terminate();
    
    //// 
    
    /**
     * Returns the currently executing control block,
     */
    ControlBlock* currentControlBlock();
    
    /**
     * The fiber system this executor is attached to.
     */
    System* const system;

    /**
     * Returns the current executor, or nullptr if this is thread does not have an associated executor.
     */
    static Executor* current();
    
private:

    /**
     * The current executor.
     */
    static thread_local Executor* current_;
    
    /**
     * Switches to the next fiber from a fiber. You must hold the control block mutex.
     */
    void switchFromRunning();
    
    /**
     * Switches to the next fiber.
     */
    void switchFromTerminated();
    
    /**
     * Jumps to the idle loop.
     */
    void jumpToIdle(boost::context::fcontext_t* stash);
    
    /**
     * Jumps to the given fiber.
     */
    void jumpToFiber(boost::context::fcontext_t* stash, ControlBlock* controlBlock);

    /**
     * Performs cleanup after a jump.
     */
    void afterJump();

    /**
     * Called when the executor has nothing to do.
     */
    void idle();
    
    /**
     * Trampoline used to start a fiber.
     */
    static void fiberRunner(intptr_t);
    
    /**
     * Stack allocator.
     */
    boost::context::fixedsize_stack stackAllocator;
    
    /**
     * The thread this executor is running on.
     */
    std::thread thread;
    
    /**
     * Scheduled control blocks waiting to be executed.
     */
    boost::lockfree::queue<ControlBlock*> runQueue;
    
    /**
     * Context executed when we have nothing to do.
     */
    boost::context::fcontext_t idleContext;
    
    /**
     * A context used just because boost requires some context to save the current state.
     */
    boost::context::fcontext_t dummyContext;
    
    /**
     * Previously executing fiber.
     */
    ControlBlock* previousControlBlock_;
        
    /**
     * The currently executing control block.
     */
    ControlBlock* currentControlBlock_;
    
    /**
     * Random engine used for work stealing.
     */
    std::mt19937 randomEngine;
    
    /**
     * Index of this scheduler.
     */
    const uint32_t myIndex;
    
    /**
     * Stack allocator.
     */
    std::unique_ptr<detail::StackPool> stackPool;
    
    bool emergencyStop;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_EXECUTOR_HPP
