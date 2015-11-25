#ifndef FIBERIZE_SCHEDULER_HPP
#define FIBERIZE_SCHEDULER_HPP

#include <fiberize/detail/controlblock.hpp>
#include <fiberize/io/detail/iocontext.hpp>

namespace fiberize {

class FiberSystem;

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
     * Enable a suspended control block.
     */
    virtual void enable(detail::ControlBlock* controlBlock, boost::unique_lock<detail::ControlBlockMutex>&& lock);

    /**
     * Enable a suspended fiber control block.
     */
    virtual void enableFiber(detail::FiberControlBlock* controlBlock, boost::unique_lock<detail::ControlBlockMutex>&& lock) = 0;

    /**
     * Suspend the currently running fiber/thread.
     */
    virtual void suspend(boost::unique_lock<detail::ControlBlockMutex>&& lock) = 0;

    /**
     * Yield CPU to other fibers or the OS.
     */
    virtual void yield(boost::unique_lock<detail::ControlBlockMutex>&& lock) = 0;

    /**
     * Terminate the currently running fiber/thread.
     */
    [[ noreturn ]] virtual void terminate() = 0;

    /**
     * Tries to steal a fiber from this task.
     */
    virtual bool tryToStealTask(detail::FiberControlBlock*& controlBlock) = 0;

    /**
     * Returns the currently executing control block. Not thread safe.
     */
    virtual detail::ControlBlock* currentControlBlock() = 0;

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
