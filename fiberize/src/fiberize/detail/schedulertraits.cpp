/**
 * Starting tasks on different schedulers.
 *
 * @file schedulertraits.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/detail/schedulertraits.hpp>
#include <fiberize/detail/singletaskscheduler.hpp>
#include <fiberize/detail/task.hpp>
#include <fiberize/context.hpp>

#include <thread>

namespace fiberize {
namespace detail {

void MultiTaskSchedulerTraits::runTask(Task* task) {
    task->grab();
    std::unique_lock<detail::TaskMutex> lock(task->mutex);
    context::detail::resume(task, std::move(lock));
}

void SingleTaskSchedulerTraits::runTask(Task* task) {
    task->grab();

    FiberSystem* system = context::system();
    std::uniform_int_distribution<uint64_t> seedDist;
    uint64_t seed = seedDist(context::random());

    std::thread thread([system, seed, task] () {
        (new SingleTaskScheduler(system, seed, task))->makeCurrent();
        task->runnable->run();
    });
    thread.detach();
}

} // namespace detail
} // namespace fiberize
