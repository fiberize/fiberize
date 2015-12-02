/**
 * User space condition variable implementation.
 *
 * @file condition.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_CONDITION_HPP
#define FIBERIZE_CONDITION_HPP

#include <mutex>

#include <fiberize/mutex.hpp>
#include <fiberize/detail/lazydeque.hpp>

namespace fiberize {

/**
 * Fully user space condition variable.
 * @note This can be only used with threads managed by fiberize, as it uses the fiberize
 *       scheduler to suspend tasks, instead of calling out to the kernel.
 * @warning No thread must be waiting when the condition is destroyed.
 */
class Condition {
public:
    Condition();
    Condition(const Condition&) = delete;
    Condition(Condition&&) = default;

    /**
     * Await the condition.
     * @note When the task is waiting for the condition it will process events.
     * @note In case of any exception, await() will have no effect on the condition
     *       and the @arg lock will be locked.
     */
    void await(std::unique_lock<Spinlock>& lock);

    /**
     * Wake up one thread waiting on the condition.
     */
    void signal(std::unique_lock<Spinlock>& lock);

    /**
     * Wake up all threads waiting on the condition.
     */
    void signalAll(std::unique_lock<Spinlock>& lock);

private:
    std::atomic<uint64_t> released;
    uint64_t nextTicket;
    detail::LazyDeque<detail::Task*> queue;
};

} // namespace fiberize

#endif // FIBERIZE_CONDITION_HPP
