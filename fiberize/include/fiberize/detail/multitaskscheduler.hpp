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

/**
 * @ingroup lifecycle
 */
class MultiTaskScheduler : public Scheduler {
public:
    MultiTaskScheduler(FiberSystem* system, uint64_t seed);
    virtual ~MultiTaskScheduler();

    void start();
    void stop();

    void resume(Task* task, std::unique_lock<TaskMutex> lock);
    void suspend() override;
    void yield() override;
    Task* currentTask() override;
    bool isMultiTasking() override;

private:
    std::thread thread;
    std::atomic<bool> stopping;

    std::deque<Task*> softTasks;
    Spinlock softMutex;
    void dequeueSoft(Task*& task);
    void stealSoft(Task*& task);

    std::deque<Task*> hardTasks;
    Spinlock hardMutex;
    void dequeueHard(Task*& task);
    void stealHard(Task*& task);

    enum Priority : uint8_t {
        Soft, Hard
    };

    void dequeue(Task*& task, Priority priority);
    void steal(Task*& task, Priority priority);

    void finishSuspending();
    static void ownedLoop();
    static void unownedLoop();

    struct UnownedContext {
        boost::context::fcontext_t context;
        boost::context::stack_context stack;
    };

    uint64_t sameStreak;
    Task* suspendingTask;
    Task* currentTask_;
    UnownedContext* unowned;
    boost::context::fcontext_t initialContext;

    std::vector<UnownedContext*> stash;
    UnownedContext* stashGet();
    void stashPut(UnownedContext* context);
    void stashClear();

#ifdef FIBERIZE_SEGMENTED_STACKS
    boost::context::segmented_stack stackAllocator;
#else
    boost::context::fixedsize_stack stackAllocator;
#endif

};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_MULTITASKSCHEDULER_HPP
