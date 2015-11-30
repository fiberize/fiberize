/**
 * Singletasking scheduler.
 *
 * @file singletaskscheduler.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_SINGLETASKSCHEDULER_HPP
#define FIBERIZE_DETAIL_SINGLETASKSCHEDULER_HPP

#include <fiberize/scheduler.hpp>

#include <boost/thread.hpp>

namespace fiberize {
namespace detail {

/**
 * @ingroup lifecycle
 */
class SingleTaskScheduler : public Scheduler {
public:
    SingleTaskScheduler(FiberSystem* system, uint64_t seed, Task* task);
    virtual ~SingleTaskScheduler();

    void resume(std::unique_lock<TaskMutex> lock);

    // Scheduler
    void suspend(std::unique_lock<TaskMutex> lock) override;
    void yield(std::unique_lock<TaskMutex> lock) override;
    void terminate(std::unique_lock<TaskMutex> lock) override;
    detail::Task* currentTask() override;
    bool isMultiTasking() override;

private:
    Task* task_;
    std::unique_lock<detail::TaskMutex> transferredLock;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_SINGLETASKSCHEDULER_HPP
