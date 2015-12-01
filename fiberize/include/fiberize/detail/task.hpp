/**
 * Structures containing the state of an executing task.
 *
 * @file task.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_TASK_HPP
#define FIBERIZE_DETAIL_TASK_HPP

#include <fiberize/path.hpp>
#include <fiberize/mailbox.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/promise.hpp>
#include <fiberize/spinlock.hpp>
#include <fiberize/detail/runnable.hpp>
#include <fiberize/detail/refrencecounted.hpp>

#include <iostream>
#include <limits>
#include <unordered_map>
#include <mutex>

#include <boost/atomic/atomic.hpp>
#include <boost/context/all.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

namespace fiberize {

class Runnable;
class Scheduler;

namespace detail {

/**
 * @defgroup lifecycle Schedulers and lifecycle of a task
 *
 * Each of the four task types (fiber, actor, future, thread) follows the same lifycycle, with minor variations.
 *
 * Tasks are executed by schedulers. There are two types of schedulers:
 *  - Singletask scheduler, which executes only a single fiber, from start to finish.
 *  - Multitask scheduler, which switches between fibers and balances its work queue with other multitask schedulers.
 * A task can be pinned, which means that it can only be run by a particular scheduler. If a task is not pinned,
 * it can be migrated between schedulers freely.
 *
 * When you create a task, it starts in the state Starting and gets passed to a scheduler. Once a scheduler picks up a newly
 * created task it changes its status to Running and executes the function associated with the task. In the case of fibers,
 * threads and futures after the function finishes the task's state is changed to Dead, it's closure is destroyed and it
 * doesn't get rescheduled any more.
 *
 * The situation is different with actors: after an actor's function finishes, it enters the Listening state, during which
 * it keeps processing incoming events. Each time an event arrives the actor's state is temporarily changed to Running
 * and after the event is processed the actor goes back to Listening. The process goes on until the actor requests to be
 * terminated, or throws an unhandled exception, which changes the state to Dead.
 *
 * If any task or an actor in the Listening mode processing an event blocks or yields, it will become Suspended. Once some condition
 * is satisfied (or instantly in the case of yield()) it will be rescheduled and when a scheduler picks up the task it will be
 * resumed.
 *
 */
///@{

/**
 * Status of a task.
 */
enum TaskStatus : uint8_t {
    /**
     * The task was newly created.
     */
    Starting,

    /**
     * The task is runnning.
     */
    Running,

    /**
     * The task is suspended, awaiting some condition.
     */
    Suspended,

    /**
     * The task is listening for events.
     */
    Listening,

    /**
     * The task is finished.
     */
    Dead
};

using TaskMutex = Spinlock;
using HandlerBlock = std::vector<std::unique_ptr<detail::Handler>>;

class Task : public ReferenceCountedAtomic {
public:
    Task() {
        status = Starting;
        scheduled = false;
        handlersInitialized = false;
        reschedule = false;
        stopped = false;
        resumes = 0;
    }

    virtual ~Task() {}

    /**
     * Status of this task.
     */
    TaskStatus status;

    /**
     * Whether the task is scheduled to be run.
     */
    bool scheduled;

    /**
     * Lock used during status change.
     */
    TaskMutex mutex;

    /**
     * Path to this task.
     */
    Path path;

    /**
     * Scheduler this task is pinned to, or nullptr.
     */
    Scheduler* pin;
    
    /**
     * Mailbox attached to this task.
     */
    std::unique_ptr<Mailbox> mailbox;

    /**
     * Whether the standard event handlers were initialized.
     */
    bool handlersInitialized;

    /**
     * Hash map of event handlers.
     */
    std::unordered_map<Path, detail::HandlerBlock, boost::hash<Path>> handlers;

    /**
     * Function used to execute this task.
     */
    std::unique_ptr<detail::ErasedRunnable> runnable;

    /**
     * The last saved context.
     */
    boost::context::fcontext_t context;

    /**
     * Tracks how many times there was an attempt to resume this task.
     */
    uint64_t resumes;

    /**
     * Used by the scheduler to know whether a task needs to be rescheduled.
     */
    uint64_t resumesExpected;

    /**
     * Whether a block should be rescheduled after a jump.
     */
    bool reschedule;

    /**
     * Whether a listening task was stopped.
     */
    bool stopped;
};

template <typename A>
class Future : public Task {
public:
    /**
     * Promise that will contain the result of this future.
     */
    Promise<Result<A>> result;
};

///@}

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_TASK_HPP
