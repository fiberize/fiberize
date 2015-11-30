/**
 * Multitasking scheduler.
 *
 * @file multitaskscheduler.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/detail/multitaskscheduler.hpp>
#include <fiberize/detail/stackpool.hpp>
#include <fiberize/fibersystem.hpp>

#include <thread>
#include <chrono>

using namespace std::literals;

namespace fiberize {
namespace detail {

MultiTaskScheduler::MultiTaskScheduler(FiberSystem* system, uint64_t seed, uint32_t index)
    : Scheduler(system, seed)
    , previousTask_(nullptr)
    , currentTask_(nullptr)
    , myIndex(index)
    , stackPool(new DefaultStackPool)
    , emergencyStop(false)
    {}

MultiTaskScheduler::~MultiTaskScheduler() {}

void MultiTaskScheduler::start() {
    executorThread = std::thread(&MultiTaskScheduler::idle, this);
}

void MultiTaskScheduler::stop() {
    emergencyStop = true;
    executorThread.join();
}

void MultiTaskScheduler::resume(Task* task, std::unique_lock<TaskMutex> lock) {
    assert(lock.owns_lock());
    assert(task->status == Starting || task->status == Suspended);
    assert(!task->scheduled);
    task->scheduled = true;
    lock.unlock();

    std::unique_lock<boost::mutex> tasksLock(tasksMutex);
    tasks.push_back(task);
}

void MultiTaskScheduler::suspend(std::unique_lock<TaskMutex> lock) {
    assert(lock.owns_lock());
    assert(currentTask_->status == Running);
    currentTask_->reschedule = false;

    /**
     * Switch to the next task.
     */
    switchFromRunning(std::move(lock));
}

void MultiTaskScheduler::yield(std::unique_lock<detail::TaskMutex> lock) {
    assert(lock.owns_lock());
    assert(currentTask_->status == Running);
    currentTask_->reschedule = true;

    /**
     * Switch to the next task.
     */
    switchFromRunning(std::move(lock));
}

void MultiTaskScheduler::terminate() {
    assert(currentTask_->status == Running);

    /**
     * Destroy the task.
     */
    currentTask_->status = Dead;
    currentTask_->runnable.reset();
    currentTask_->handlers.clear();
    stackPool->delayedDeallocate(currentTask_->stack);
    currentTask_->drop();
    currentTask_ = nullptr;

    /**
     * Switch to the next fiber.
     */
    switchFromTerminated();

    /**
     * The jump doesn't return.
     */
    __builtin_unreachable();
}

bool MultiTaskScheduler::tryToStealTask(Task*& task) {
    std::unique_lock<boost::mutex> lock(tasksMutex);
    if (tasks.size() > 0 && !tasks[0]->pin) {
        task = tasks.front();
        tasks.pop_front();
        return true;
    } else if (tasks.size() > 1 && !tasks[1]->pin) {
        task = tasks[1];
        tasks[1] = tasks[0];
        tasks.pop_front();
        return true;
    } else {
        return false;
    }
}

Task* MultiTaskScheduler::currentTask() {
    return currentTask_;
}

bool MultiTaskScheduler::isMultiTasking() {
    return true;
}

bool MultiTaskScheduler::tryDequeue(Task*& task) {
    if (emergencyStop) return false;

    std::unique_lock<boost::mutex> lock(tasksMutex);
    if (!tasks.empty()) {
        task = tasks.back();
        tasks.pop_back();
        return true;
    } else {
        return false;
    }
}

void MultiTaskScheduler::idle() {
    makeCurrent();

    /**
     * Idle loop.
     */
    Task* task;
    std::uniform_int_distribution<uint32_t> randomFiberScheduler(0, system()->schedulers().size() - 2);

    while (!emergencyStop) {
        while (tryDequeue(task)) {
            assert(task->status == Starting || task->status == Suspended);
            assert(task->scheduled);
            jumpToFiber(&idleContext, task);
        }

        if (system()->schedulers().size() > 1) {
            /**
             * Choose a random executor that is not ourself.
             */
            uint32_t i = randomFiberScheduler(random());
            if (i >= myIndex)
                i += 1;

            /**
             * Try to take his job.
             */
            if (system()->schedulers()[i]->tryToStealTask(task)) {
                assert(task->status == Starting || task->status == Suspended);
                assert(task->scheduled);
                jumpToFiber(&idleContext, task);
            } else {
                if (!ioContext().poll())
                    std::this_thread::sleep_for(1ns);
            }
        } else {
            std::this_thread::sleep_for(1ns);
        }
    }
}

void MultiTaskScheduler::switchFromRunning(std::unique_lock<TaskMutex> lock) {
    assert(lock.owns_lock());
    Task* task;

    if (tryDequeue(task)) {
        assert(task->status == Starting || task->status == Suspended);
        assert(task->scheduled);

        /**
         * Make sure we don't context switch into the same context.
         */
        if (task == currentTask_) {
            task->status = Running;
            task->scheduled = false;
            return;
        } else {
            previousTaskLock = std::move(lock);

            /**
             * Switch the current control block to the next fiber and make the jump
             * saving our current state to the control block.
             */
            jumpToFiber(&currentTask_->context, task);
        }
    } else {
        previousTaskLock = std::move(lock);

        /**
         * Jump to the idle context.
         */
        jumpToIdle(&currentTask_->context);
    }
}

void MultiTaskScheduler::switchFromTerminated() {
    Task* task;

    if (tryDequeue(task)) {
        assert(task->status == Starting || task->status == Suspended);
        assert(task->scheduled);

        /**
         * Switch the current control block to the next fiber and make the jump
         * saving our current state to the control block.
         */
        jumpToFiber(&dummyContext, task);

    } else {
        /**
         * Jump to the idle context.
         */
        jumpToIdle(&dummyContext);
    }

    /**
     * Both jumps cannot return.
     */
    __builtin_unreachable();
}

void MultiTaskScheduler::jumpToIdle(boost::context::fcontext_t* stash) {
    previousTask_ = currentTask_;
    currentTask_ = nullptr;

    boost::context::jump_fcontext(stash, idleContext, 0);
    static_cast<MultiTaskScheduler*>(current())->afterJump();
}

void MultiTaskScheduler::jumpToFiber(boost::context::fcontext_t* stash, Task* task) {
    previousTask_ = currentTask_;
    currentTask_ = task;

    if (previousTask_ != nullptr) {
        assert(previousTaskLock.owns_lock());
    }

    if (currentTask_->status == Starting) {
        currentTask_->stack = stackPool->allocate();
        currentTask_->context = boost::context::make_fcontext(currentTask_->stack.sp, currentTask_->stack.size, &MultiTaskScheduler::fiberRunner);
    }

    boost::context::jump_fcontext(stash, currentTask_->context, 0);
    static_cast<MultiTaskScheduler*>(current())->afterJump();
}

void MultiTaskScheduler::afterJump() {
    if (previousTask_ != nullptr) {
        /**
         * We jumped from a fiber and are holding the mutex on it.
         * Suspend that fiber and drop the reference.
         */
        assert(previousTaskLock.owns_lock());
        previousTask_->status = Suspended;

        if (previousTask_->reschedule) {
            previousTask_->scheduled = true;
            previousTaskLock.unlock();

            std::unique_lock<boost::mutex> taskLock(tasksMutex);
            tasks.push_front(previousTask_);
        } else {
            previousTask_->scheduled = false;
            previousTaskLock.unlock();
        }

        assert(!previousTaskLock.owns_lock());
        previousTask_ = nullptr;
    } else {
        assert(!previousTaskLock.owns_lock());
    }

    if (currentTask_ != nullptr) {
        std::unique_lock<TaskMutex> lock(currentTask_->mutex);
        assert(currentTask_->status == Starting || currentTask_->status == Suspended);
        assert(currentTask_->scheduled);
        currentTask_->status = Running;
        currentTask_->scheduled = false;
    }

    ioContext().throttledPoll();
}

void MultiTaskScheduler::fiberRunner(intptr_t) {
    /**
      * Change the status to Running.
      */
    auto scheduler = static_cast<MultiTaskScheduler*>(current());
    scheduler->afterJump();

    /**
     * Execute the fiber.
     */
    auto task = scheduler->currentTask_;
    task->runnable->run();
}

} // namespace detail
} // namespace fiberize
