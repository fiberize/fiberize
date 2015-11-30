/**
 * Multitasking scheduler.
 *
 * @file multitaskscheduler.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_MULTITASKSCHEDULER_HPP
#define FIBERIZE_DETAIL_MULTITASKSCHEDULER_HPP

#include <thread>

#include <fiberize/scheduler.hpp>

namespace fiberize {
namespace detail {

class StackPool;

class MultiTaskScheduler : public Scheduler {
public:
    MultiTaskScheduler(FiberSystem* system, uint64_t seed, uint32_t index);
    virtual ~MultiTaskScheduler();

    void start();
    void stop();

    void resume(Task* task, std::unique_lock<TaskMutex> lock);
    void suspend(std::unique_lock<TaskMutex> lock) override;
    void yield(std::unique_lock<TaskMutex> lock) override;
    [[ noreturn ]] void terminate() override;
    Task* currentTask() override;
    bool isMultiTasking() override;

    bool tryToStealTask(Task*& task);

private:
    bool tryDequeue(Task*& task);

    /**
     * Switches to the next task from a task. You must hold the control block mutex.
     */
    void switchFromRunning(std::unique_lock<TaskMutex> lock);

    /**
     * Switches to the next task.
     */
    [[ noreturn ]] void switchFromTerminated();

    /**
     * Jumps to the idle loop.
     */
    void jumpToIdle(boost::context::fcontext_t* stash);

    /**
     * Jumps to the given task.
     */
    void jumpToFiber(boost::context::fcontext_t* stash, Task* task);

    /**
     * Performs cleanup after a jump.
     */
    void afterJump();

    /**
     * Called when the executor has nothing to do.
     */
    void idle();

    /**
     * Trampoline used to start a task.
     */
    static void fiberRunner(intptr_t);

    /**
     * Stack allocator.
     */
    boost::context::fixedsize_stack stackAllocator;

    /**
     * The thread this scheduler is running on.
     */
    std::thread executorThread;

    /**
     * Scheduled tasks waiting to be executed.
     */
    std::deque<Task*> tasks;
    boost::mutex tasksMutex;

    /**
     * Context executed when we have nothing to do.
     */
    boost::context::fcontext_t idleContext;

    /**
     * A context used just because boost requires some context to save the current state.
     */
    boost::context::fcontext_t dummyContext;

    /**
     * Previously executing task.
     */
    Task* previousTask_;

    /**
     * Variable used to transport the lock during a context switch.
     */
    std::unique_lock<TaskMutex> previousTaskLock;

    /**
     * The currently executing task.
     */
    Task* currentTask_;

    /**
     * Index of this scheduler.
     */
    const uint32_t myIndex;

    /**
     * Stack allocator.
     */
    std::unique_ptr<StackPool> stackPool;

    bool emergencyStop;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_MULTITASKSCHEDULER_HPP
