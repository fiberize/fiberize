#include <fiberize/context.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/exceptions.hpp>
#include <fiberize/events.hpp>
#include <fiberize/detail/task.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/singletaskscheduler.hpp>
#include <fiberize/detail/multitaskscheduler.hpp>
#include <fiberize/fibersystem.hpp>

namespace fiberize {
namespace context {

FiberSystem* system() {
    return Scheduler::current()->system();
}

Scheduler* scheduler() {
    return Scheduler::current();
}

void yield() {
    Scheduler::current()->yield(std::unique_lock<fiberize::detail::TaskMutex>(detail::task()->mutex));
}

void process() {
    PendingEvent event;
    auto task = detail::task();
    std::unique_lock<fiberize::detail::TaskMutex> lock(task->mutex);
    while (task->mailbox->dequeue(event)) {
        lock.unlock();
        try {
            detail::dispatchEvent(event);
        } catch (...) {
            if (event.freeData)
                event.freeData(event.data);
            throw;
        }
        if (event.freeData)
            event.freeData(event.data);
        lock.lock();
    }
}

void processForever() {
    const bool condition = false;
    processUntil(condition);

    /**
     * Obviously, the condition will always be false.
     */
    __builtin_unreachable();
}

static void initializeHandlers() {
    /**
     * This probably could be improved a lot.
     */
    auto task = detail::task();
    std::unique_ptr<fiberize::detail::Handler> killHandler(new fiberize::detail::TypedHandler<void>([] () {
        throw Killed();
    }));
    killHandler->grab();
    task->handlers[kill.path()].emplace_back(std::move(killHandler));
    task->handlersInitialized = true;
}

void processUntil(const bool& condition) {
    auto task = detail::task();
    PendingEvent event;
    while (!condition) {
        /**
         * First, process all pending events.
         */
        PendingEvent event;
        std::unique_lock<fiberize::detail::TaskMutex> lock(task->mutex);
        while (task->mailbox->dequeue(event)) {
            if (!task->handlersInitialized)
                initializeHandlers();

            lock.unlock();
            try {
                detail::dispatchEvent(event);
            } catch (...) {
                if (event.freeData)
                    event.freeData(event.data);
                throw;
            }
            if (event.freeData)
                event.freeData(event.data);

            /**
             * Short-circuit when condition is triggered.
             */
            if (condition) {
                return;
            }

            lock.lock();
        }

        /**
         * No more events, we can suspend the thread.
         */
        scheduler()->suspend(std::move(lock));
    }
}

FiberRef self() {
    return FiberRef(std::make_shared<fiberize::detail::LocalFiberRef>(system(), detail::task()));
}

namespace detail {

fiberize::detail::Task* task() {
    return scheduler()->currentTask();
}

static void collectGarbage(fiberize::detail::HandlerBlock& block) {
    size_t i = 0;

    const size_t n = block.size();
    for (size_t j = 0; j < n; ++j) {
        if (!block[j]->isDestroyed()) {
            block[i].reset(block[j].release());
            ++i;
        }
    }

    block.resize(i);
}

void dispatchEvent(const PendingEvent& event) {
    auto task = detail::task();

    /**
     * Find a handler block.
     */
    auto blockIt = task->handlers.find(event.path);
    if (blockIt == task->handlers.end())
        return;
    fiberize::detail::HandlerBlock& block = blockIt->second;

    /**
     * GC dead handlers.
     */
    collectGarbage(block);

    /**
     * There are no alive handlers, remove the handler block.
     */
    if (block.empty()) {
        task->handlers.erase(blockIt);
        return;
    }

    /**
     * Execute the handlers.
     */
    auto it = block.rbegin();
    auto end = block.rend();
    while (it != end) {
        (*it)->execute(event.data);
        ++it;
    }
}

HandlerRef bind(const Path& path, std::unique_ptr<fiberize::detail::Handler> handler) {
    HandlerRef ref(handler.get());
    task()->handlers[path].emplace_back(std::move(handler));
    return ref;
}

void resume(fiberize::detail::Task* task, std::unique_lock<fiberize::detail::TaskMutex> lock) {
    assert(lock.owns_lock());

    Scheduler* sched;
    bool knownMultiTasking = false;

    /**
     * Do not resume a scheduled or running task.
     */
    if ((task->status != fiberize::detail::Suspended && task->status != fiberize::detail::Starting) || task->scheduled)
        return;

    if (task->pin != nullptr) {
        /**
         * Forward pinned tasks to their scheduler.
         */
        sched = task->pin;
    } else {
        /**
         * If the current scheduler is a multi tasking one, us it. Otherwise pick a random
         * multitasking scheduler.
         */
        if (scheduler()->isMultiTasking()) {
            sched = scheduler();
        } else {
            std::uniform_int_distribution<size_t> dist(0, system()->schedulers().size() - 1);
            size_t index = dist(scheduler()->random());
            sched = system()->schedulers()[index];
        }
        knownMultiTasking = true;
    }

    if (knownMultiTasking || sched->isMultiTasking()) {
        static_cast<fiberize::detail::MultiTaskScheduler*>(sched)->resume(task, std::move(lock));
    } else {
        static_cast<fiberize::detail::SingleTaskScheduler*>(sched)->resume(std::move(lock));
    }
}

void terminate() {
    scheduler()->terminate();
}

} // namespace detail

} // namespace context
} // namespace fiberize
