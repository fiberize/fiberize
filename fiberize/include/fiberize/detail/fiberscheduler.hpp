#ifndef FIBERIZE_DETAIL_FIBERSCHEDULER_HPP
#define FIBERIZE_DETAIL_FIBERSCHEDULER_HPP

#include <thread>

#include <fiberize/scheduler.hpp>

namespace fiberize {
namespace detail {

class StackPool;

class FiberScheduler : public Scheduler {
public:
    FiberScheduler(FiberSystem* system, uint64_t seed, uint32_t index);
    virtual ~FiberScheduler();

    void start();
    void stop();

    void enableFiber(FiberControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock) override;
    void suspend(boost::unique_lock<ControlBlockMutex>&& lock) override;
    void yield(boost::unique_lock<ControlBlockMutex>&& lock) override;
    [[ noreturn ]] void terminate() override;
    bool tryToStealTask(FiberControlBlock*& controlBlock) override;
    detail::ControlBlock* currentControlBlock() override;

private:
    bool tryDequeue(FiberControlBlock*& controlBlock);

    /**
     * Switches to the next fiber from a fiber. You must hold the control block mutex.
     */
    void switchFromRunning(boost::unique_lock<ControlBlockMutex>&& lock);

    /**
     * Switches to the next fiber.
     */
    [[ noreturn ]] void switchFromTerminated();

    /**
     * Jumps to the idle loop.
     */
    void jumpToIdle(boost::context::fcontext_t* stash);

    /**
     * Jumps to the given fiber.
     */
    void jumpToFiber(boost::context::fcontext_t* stash, FiberControlBlock* controlBlock);

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
    std::deque<FiberControlBlock*> tasks;
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
     * Previously executing fiber.
     */
    FiberControlBlock* previousControlBlock_;

    /**
     * Variable used to transport the lock during a context switch.
     */
    boost::unique_lock<detail::ControlBlockMutex> previousControlBlockLock;

    /**
     * The currently executing control block.
     */
    FiberControlBlock* currentControlBlock_;

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

#endif // FIBERIZE_DETAIL_FIBERSCHEDULER_HPP
