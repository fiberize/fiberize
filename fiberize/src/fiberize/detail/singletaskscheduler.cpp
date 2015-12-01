/**
 * Singletasking scheduler.
 *
 * @file singletaskscheduler.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/detail/singletaskscheduler.hpp>
#include <fiberize/fibersystem.hpp>

#include <thread>

namespace fiberize {
namespace detail {

SingleTaskScheduler::SingleTaskScheduler(FiberSystem* system, uint64_t seed, Task* task)
    : Scheduler(system, seed)
    , task_(task) {
    task->grab();
    task->pin = this;
}

SingleTaskScheduler::~SingleTaskScheduler() {
    if (task_ != nullptr) {
        task_->pin = nullptr;
        task_->drop();
    }
}

void SingleTaskScheduler::resume(std::unique_lock<TaskMutex> lock) {
    assert(task_->status == Suspended || task_->status == Listening);
    assert(!task_->scheduled);
    task_->status = Running;
    task_->scheduled = false;
    task_->resumes += 1;
    resumed.store(true, std::memory_order_release);
    lock.unlock();
}

void SingleTaskScheduler::suspend() {
    std::unique_lock<TaskMutex> lock(task_->mutex);
    assert(task_->status == Running);
    assert(!task_->scheduled);

    // Resume instantly if we have something to process.
    if (task_->resumes != task_->resumesExpected) {
        return;
    }

    task_->status = Suspended;
    resumed.store(false, std::memory_order_relaxed);
    lock.unlock();

    /**
     * Loop until someone breaks it.
     */
    uint64_t idleStreak = 0;
    while (!resumed.load(std::memory_order_acquire)) {
        if (ioContext().poll()) {
            idleStreak = 0;
        } else {
            idle(idleStreak);
        }
    }
}

void SingleTaskScheduler::yield() {
    std::this_thread::yield();
}

Task* SingleTaskScheduler::currentTask() {
    return task_;
}

bool SingleTaskScheduler::isMultiTasking() {
    return false;
}

} // namespace detail
} // namespace fiberize
