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
    , task_(task)
    {}

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

void SingleTaskScheduler::terminate() {
    task_->status = Dead;
    task_->runnable.reset();
    task_->handlers.clear();
    task_->drop();
    task_ = nullptr;
    int retval = 0;

    // NOTE: AFAIK there is no equivalent to this function in boost::thread or std.
    //       This is not an issue for now, because we only target linux.
    pthread_exit(&retval);
}

Task* SingleTaskScheduler::currentTask() {
    return task_;
}

bool SingleTaskScheduler::isMultiTasking() {
    return false;
}

} // namespace detail
} // namespace fiberize
