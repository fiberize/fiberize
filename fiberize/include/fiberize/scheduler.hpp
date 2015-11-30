/**
 * Task scheduler.
 *
 * @file scheduler.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_SCHEDULER_HPP
#define FIBERIZE_SCHEDULER_HPP

#include <fiberize/detail/task.hpp>
#include <fiberize/io/detail/iocontext.hpp>

namespace fiberize {

class FiberSystem;

/**
 * @ingroup lifecycle
 */
class Scheduler {
public:
    /**
     * Initializes the scheduler with the given fiber system and random generator seed.
     */
    Scheduler(FiberSystem* system, uint64_t seed);

    /**
     * Destroys the scheduler, without deactivating it..
     */
    virtual ~Scheduler();

    /**
     * Sets this object as the thread local scheduler.
     */
    void makeCurrent();

    /**
     * Sets the current scheduler to nullptr.
     */
    static void resetCurrent();

    /**
     * Suspend the currently running task.
     */
    virtual void suspend(std::unique_lock<detail::TaskMutex> lock) = 0;

    /**
     * Yield CPU to other tasks or the OS, but eventually reschedule this task.
     */
    virtual void yield(std::unique_lock<detail::TaskMutex> lock) = 0;

    /**
     * Terminate the currently running task.
     */
    virtual void terminate(std::unique_lock<detail::TaskMutex> lock) = 0;

    /**
     * Returns the currently executing task.
     */
    virtual detail::Task* currentTask() = 0;

    /**
     * Whether the current scheduler is a MultiTaskScheduler.
     */
    virtual bool isMultiTasking() = 0;

    /**
     * Returns the fiber system this scheduler is attached to.
     */
    inline FiberSystem* system() { return system_; }

    /**
     * Returns the IO context associated with this scheduler.
     */
    inline io::detail::IOContext& ioContext() { return ioContext_; }

    /**
     * Returns a random generator local to this scheduler.
     */
    inline std::mt19937_64& random() { return random_; }

    /**
     * Returns the scheduler attached to this thread.
     */
    static inline Scheduler* current() { return current_; }

private:
    FiberSystem* system_;
    io::detail::IOContext ioContext_;
    std::mt19937_64 random_;
    static thread_local Scheduler* current_;
};

} // namespace fiberize

#endif // FIBERIZE_SCHEDULER_HPP
