/**
 * Starting tasks on different schedulers.
 *
 * @file runner.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/detail/runner.hpp>
#include <fiberize/detail/singletaskscheduler.hpp>
#include <fiberize/detail/task.hpp>
#include <fiberize/context.hpp>

#include <thread>

namespace fiberize {
namespace detail {

void runTaskAsMicrothread(Task* task) {
    std::unique_lock<TaskMutex> lock(task->mutex);
    context::detail::resume(task, std::move(lock));
}

void runTaskAsOSThread(Task* task) {
    FiberSystem* system = context::system();
    std::uniform_int_distribution<uint64_t> seedDist;
    uint64_t seed = seedDist(context::random());

    std::thread thread([system, seed, task] () {
        // Create the scheduler for this task.
        std::unique_ptr<SingleTaskScheduler> scheduler{new SingleTaskScheduler(system, seed, task)};
        scheduler->makeCurrent();

        // Run the task. This doesn't throw.
        task->runnable->run();

        // Process events.
        try {
            uint64_t idleStreak = 0;
            while (!task->stopped) {
                scheduler->idle(idleStreak);

                std::unique_lock<TaskMutex> lock(task->mutex);
                if (!task->mailbox->empty()) {
                    context::detail::process(lock);
                    idleStreak = 0;
                }
                lock.unlock();

                if (scheduler->ioContext().poll()) {
                    idleStreak = 0;
                }
            }
        } catch (...) {
            // Nothing,
        }

        std::unique_lock<TaskMutex> lock(task->mutex);
        scheduler->kill(task, std::move(lock));
        scheduler->resetCurrent();
    });
    thread.detach();
}

} // namespace detail
} // namespace fiberize
