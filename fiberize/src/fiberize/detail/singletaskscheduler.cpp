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
    task->pin = this;
}

SingleTaskScheduler::~SingleTaskScheduler() {
    if (task_ != nullptr)
        task_->drop();
}

void SingleTaskScheduler::resume(std::unique_lock<TaskMutex> lock) {
    assert(task_->status == Suspended);
    assert(!task_->scheduled);
    task_->scheduled = true;

    /**
     * Transfer the lock.
     */
    assert(!transferredLock.owns_lock());
    transferredLock = std::move(lock);
    ioContext().stopLoop();
}

void SingleTaskScheduler::suspend(std::unique_lock<TaskMutex> lock) {
    assert(task_->status == Running);
    task_->status = Suspended;
    assert(!task_->scheduled);
    lock.unlock();

    /**
     * Loop until someone breaks it.
     */
    ioContext().runLoop();

    /**
     * Receive the lock.
     */
    assert(transferredLock.owns_lock());
    assert(task_->status == Suspended);
    assert(task_->scheduled);
    task_->status = Running;
    task_->scheduled = false;
    transferredLock.unlock();
}

void SingleTaskScheduler::yield(std::unique_lock<TaskMutex> lock) {
    lock.unlock();
    ioContext().runLoopNoWait();
    std::this_thread::yield();
}

void SingleTaskScheduler::terminate(std::unique_lock<TaskMutex> lock) {
    /**
     * Complete the task.
     */
    assert(task_->status == Running);
    assert(!task_->scheduled);
    task_->status = Dead;
    lock.unlock();
    task_->runnable.reset();
    task_->handlers.clear();
    task_->drop();
    task_ = nullptr;

    /**
     * Delete the scheduler! It's sole purpose is complete.
     */
    delete this;

    /**
     * Exit the function. Single task scheduler's terminate should be only called when:
     *  - unfiberizing a thread, in which case we don't want to end the thread.
     *  - terminating a fiber started as a thread, in which case we're going to end the
     *    thread right after terminate()
     */
}

Task* SingleTaskScheduler::currentTask() {
    return task_;
}

bool SingleTaskScheduler::isMultiTasking() {
    return false;
}

} // namespace detail
} // namespace fiberize
