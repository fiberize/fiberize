#ifndef FIBERIZE_DETAIL_EXECUTOR_HPP
#define FIBERIZE_DETAIL_EXECUTOR_HPP

#include <thread>

#include <boost/lockfree/queue.hpp>
#include <boost/context/all.hpp>

#include <fiberize/detail/controlblock.hpp>

namespace fiberize {
namespace detail {
    
class Executor {
public:
    /**
     * Spawns the executor in a new thread.
     */
    Executor();
    
    /**
     * Schedules a fiber to be executed by this executor.
     */
    void execute(ControlBlock* controlBlock);
    
    /**
     * Suspend this fiber. This can only be executed by a fiber on tthe current executor.
     */
    void suspend();
    
    /**
     * Returns the currently executing control block,
     */
    ControlBlock* currentControlBlock();
    
    /**
     * The current executor.
     */
    static thread_local Executor* current;
    
private:
    /**
     * Executes the fibers.
     */
    void run();
    
    /**
     * The thread this executor is running on.
     */
    std::thread thread;
    
    /**
     * Control blocks waiting to be executed.
     */
    boost::lockfree::queue<ControlBlock*> runQueue;
    
    /**
     * Context returning to the executor.
     */
    boost::context::fcontext_t returnContext;
    
    /**
     * The currently executing control block.
     */
    ControlBlock* currentControlBlock_;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_EXECUTOR_HPP
