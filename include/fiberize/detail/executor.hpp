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
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_EXECUTOR_HPP
