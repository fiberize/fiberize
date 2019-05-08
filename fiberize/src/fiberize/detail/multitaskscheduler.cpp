/**
 * Advanced multitasking scheduler.
 *
 * @file multitaskscheduler.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/detail/multitaskscheduler.hpp>
#include <fiberize/fibersystem.hpp>

namespace fiberize {
namespace detail {

constexpr uint64_t sameStreakLimit = 64;
constexpr uint64_t stashSize = 256;
constexpr uint64_t stealTries = 2;

MultiTaskScheduler::MultiTaskScheduler(FiberSystem* system, uint64_t seed)
    : Scheduler(system, seed)
    , stopping(false)
    , sameStreak(0)
    , suspendingTask(nullptr)
    , currentTask_(nullptr)
    , unowned(nullptr) {
    stash.reserve(stashSize);
}

MultiTaskScheduler::~MultiTaskScheduler() {
    if (!stopping.load(std::memory_order_consume))
        stop();
}

void MultiTaskScheduler::start() {
    thread = std::thread([this] () {
        makeCurrent();
        unowned = stashGet();
        boost::context::detail::jump_fcontext(&initialContext, unowned->context);
        if (unowned != nullptr) {
            stashPut(unowned);
            unowned = nullptr;
        }
        resetCurrent();
    });
}

void MultiTaskScheduler::stop() {
    stopping.store(true, std::memory_order_release);
    thread.join();
    stashClear();
}

void MultiTaskScheduler::resume(Task* task, std::unique_lock<Spinlock> lock) {
    assert(lock.owns_lock());
    assert(task->status == Starting || task->status == Listening || task->status == Suspended);
    assert(!task->scheduled);
    TaskStatus status = task->status;
    task->resumes += 1;
    task->scheduled = true;
    lock.unlock();

    if (status == Starting || status == Listening) {
        std::unique_lock<Spinlock> lock(softMutex);
        softTasks.push_front(task);
    } else if (status == Suspended) {
        std::unique_lock<Spinlock> lock(hardMutex);
        hardTasks.push_front(task);
    } else {
        // Impossible.
        __builtin_unreachable();
    }
}

void MultiTaskScheduler::suspend() {
    ownedLoop();
}

void MultiTaskScheduler::yield() {
    currentTask_->resumesExpected = std::numeric_limits<uint64_t>::max();
    ownedLoop();
}

Task* MultiTaskScheduler::currentTask() {
    return currentTask_;
}

bool MultiTaskScheduler::isMultiTasking() {
    return true;
}

void MultiTaskScheduler::dequeueSoft(Task*& task) {
    std::unique_lock<Spinlock> lock(softMutex);
    if (!softTasks.empty()) {
        task = softTasks.front();
        softTasks.pop_front();
    }
}

void MultiTaskScheduler::stealSoft(Task*& task) {
    std::unique_lock<Spinlock> lock(softMutex);
    if (!softTasks.empty() && softTasks.back()->pin == nullptr) {
        task = softTasks.back();
        softTasks.pop_back();
    }
}

void MultiTaskScheduler::dequeueHard(Task*& task) {
    std::unique_lock<Spinlock> lock(hardMutex);
    if (!hardTasks.empty()) {
        task = hardTasks.front();
        hardTasks.pop_front();
    }
}

void MultiTaskScheduler::stealHard(Task*& task) {
    std::unique_lock<Spinlock> lock(hardMutex);
    if (!hardTasks.empty() && hardTasks.back()->pin == nullptr) {
        task = hardTasks.back();
        hardTasks.pop_back();
    }
}

void MultiTaskScheduler::dequeue(Task*& task, MultiTaskScheduler::Priority priority) {
    if (priority == Soft) {
        for (int i = 0; i < 2; ++i) {
            dequeueSoft(task); if (task) return;
            dequeueHard(task); if (task) return;
        }
    } else {
        for (int i = 0; i < 2; ++i) {
            dequeueHard(task); if (task) return;
            dequeueSoft(task); if (task) return;
        }
    }
}

void MultiTaskScheduler::steal(Task*& task, MultiTaskScheduler::Priority priority) {
    size_t n = system()->schedulers().size();
    std::uniform_int_distribution<size_t> dist(0, n-1);

    for (uint i = 0; i < stealTries; ++i) {
        size_t index = dist(random());
        auto target = system()->schedulers()[index];

        if (priority == Soft) {
            target->stealSoft(task); if (task) return;
            target->stealHard(task); if (task) return;
        } else {
            target->stealHard(task); if (task) return;
            target->stealSoft(task); if (task) return;
        }
    }
}

MultiTaskScheduler::Priority MultiTaskScheduler::choosePriority(MultiTaskScheduler::Priority preferred) {
    if (sameStreak >= sameStreakLimit) {
        sameStreak = 0;

        switch (preferred) {
            case Hard:
                return Soft;

            case Soft:
                return Hard;

            default:
                __builtin_unreachable();
        }
    } else {
        return preferred;
    }
}

void MultiTaskScheduler::finishSuspending() {
    if (suspendingTask != nullptr) {
        std::unique_lock<Spinlock> lock(suspendingTask->spinlock);
        assert(suspendingTask->status == Running);
        suspendingTask->status = Suspended;
        suspendingTask->scheduled = false;

        // Reschedule the task if required.
        if (suspendingTask->resumes != suspendingTask->resumesExpected) {
            resume(suspendingTask, std::move(lock));
        }

        suspendingTask = nullptr;
    }
}

void MultiTaskScheduler::ownedLoop() {
    MultiTaskScheduler* self = static_cast<MultiTaskScheduler*>(current());

    // If we are in the ownedLoop we must be executing a task.
    assert(self->currentTask_ != nullptr);

    // If the scheduler is stopping return to the initial context.
    if (self->stopping.load(std::memory_order_consume)) {
        boost::context::detail::jump_fcontext(&self->currentTask_->context, self->initialContext);
        return;
    }

    // Perform the periodic IO check.
    self->ioContext().throttledPoll();

    self->suspendingTask = self->currentTask_;
    self->currentTask_ = nullptr;

    Priority priority = self->choosePriority(Hard);
    // Try to find a task, prefferably a hard one.
    self->dequeue(self->currentTask_, priority);

    // If we have no tasks, try to steal some.
    if (self->currentTask_ == nullptr)
        self->steal(self->currentTask_, priority);

    // If we still don't have any task, jump into an unowned context.
    if (self->currentTask_ == nullptr) {
        self->sameStreak = 0;
        self->unowned = self->stashGet();
        boost::context::detail::jump_fcontext(&self->suspendingTask->context, self->unowned->context);
    } else {
        // We got a task, execute it.
        if (self->currentTask_->status == Starting || self->currentTask_->status == Listening) {
            // We cannot start a new task on an owned stack. Let's get a new stack and jump to it.
            self->sameStreak = 0;
            self->unowned = self->stashGet();
            boost::context::detail::jump_fcontext(&self->suspendingTask->context, self->unowned->context);
        } else if (self->currentTask_->status == Suspended) {
            // Jump back to a suspended task.
            self->sameStreak += 1;
            boost::context::detail::jump_fcontext(&self->suspendingTask->context, self->currentTask_->context);
        } else {
            // Impossible.
            __builtin_unreachable();
        }
    }

    // Restore self after running a task, in case the context got migrated.
    self = static_cast<MultiTaskScheduler*>(current());

    // We might have to finish suspending a task if one jumeped to us.
    self->finishSuspending();

    // If we jumped form an unowned context we have to stash it.
    if (self->unowned != nullptr) {
        self->stashPut(self->unowned);
        self->unowned = nullptr;
    }

    // Change the status of the current task.
    std::unique_lock<Spinlock> lock(self->currentTask_->spinlock);
    assert(self->currentTask_->status == Suspended);
    assert(self->currentTask_->scheduled);
    self->currentTask_->status = Running;
    self->currentTask_->scheduled = false;
}

void MultiTaskScheduler::unownedLoop() {
    uint64_t idleStreak = 0;
    for (;;) {
        // Refresh self, in case we got migrated.
        MultiTaskScheduler* self = static_cast<MultiTaskScheduler*>(current());

        // If the scheduler is stopping return to the initial context.
        if (self->stopping.load(std::memory_order_consume)) {
            boost::context::detail::jump_fcontext(&self->unowned->context, self->initialContext);
            continue;
        }

        // Perform the periodic IO check.
        self->ioContext().throttledPoll();

        // We might have to suspend a task, after a jump from the owned loop.
        self->finishSuspending();

        Priority priority = self->choosePriority(Soft);
        // If there was no assigned task, try to get one.
        if (self->currentTask_ == nullptr)
            self->dequeue(self->currentTask_, priority);

        // If we have no tasks, try to steal some.
        if (self->currentTask_ == nullptr)
            self->steal(self->currentTask_, priority);

        // If we still don't have any task, try again or go to sleep,
        // depending on how long are we spinning.
        if (self->currentTask_ == nullptr) {
            self->idle(idleStreak);
            continue;
        } else {
            idleStreak = 0;
        }

        TaskStatus status = self->currentTask_->status;
        if (status == Starting || status == Listening) {
            self->sameStreak += 1;

            // The context becomes owned.
            UnownedContext* unowned = self->unowned;
            self->unowned = nullptr;

            // Change the status.
            std::unique_lock<Spinlock> lock(self->currentTask_->spinlock);
            self->currentTask_->status = Running;
            self->currentTask_->scheduled = false;
            self->currentTask_->context = unowned->context;

            if (status == Starting) {
                // Execute the task.
                lock.unlock();
                self->currentTask_->runnable->run();
                lock.lock();
            } else if (status == Listening) {
                // Process events.
                try {
                    context::detail::process(lock);
                } catch (...) {
                    // Stop the task if an exception escapes.
                    assert(!lock.owns_lock());
                    lock.lock();
                    self->currentTask_->stopped = true;
                }
            } else {
                // Impossible.
                __builtin_unreachable();
            }

            // Restore self after running a task, in case the context got migrated.
            self = static_cast<MultiTaskScheduler*>(current());

            // The context becomes unowned again.
            self->unowned = unowned;

            // Change the status of the task.
            assert(lock.owns_lock());
            assert(self->currentTask_->status == Running);
            assert(!self->currentTask_->scheduled);

            // If the task is not stopped put it back to listening, otherwise kill it.
            if (!self->currentTask_->stopped) {
                self->currentTask_->status = Listening;

                // Reschedule the task if required.
                if (self->currentTask_->resumesExpected != self->currentTask_->resumes) {
                    self->resume(self->currentTask_, std::move(lock));
                } else {
                    lock.unlock();
                }
            } else {
                kill(self->currentTask_, std::move(lock));
            }

            self->currentTask_ = nullptr;
        } else if (self->currentTask_->status == Suspended) {
            self->sameStreak = 0;

            // Too bad, the task is suspended. This means we have to context switch, therefore
            // wasting our current context.
            boost::context::detail::jump_fcontext(&self->unowned->context, self->currentTask_->context);
        } else {
            // Impossible.
            __builtin_unreachable();
        }
    }
}

MultiTaskScheduler::UnownedContext* MultiTaskScheduler::stashGet() {
    UnownedContext* context;

    if (!stash.empty()) {
        context = stash.back();
        stash.pop_back();
    } else {
        // Create a new context.
        context = new UnownedContext;
        context->stack = stackAllocator.allocate();
        context->context = boost::context::detail::make_fcontext(context->stack.sp, context->stack.size, [] (boost::context::detail::transfer_t) {
            unownedLoop();
        });
    }

    return context;
}

void MultiTaskScheduler::stashPut(UnownedContext* context) {
    assert(context != nullptr);
    if (stash.size() < stashSize) {
        stash.push_back(context);
    } else {
        stackAllocator.deallocate(context->stack);
        delete context;
    }
}

void MultiTaskScheduler::stashClear() {
    for (UnownedContext* context : stash) {
        stackAllocator.deallocate(context->stack);
        delete context;
    }
    stash.clear();
}

} // namespace detail
} // namespace fiberize
