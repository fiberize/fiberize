/**
 * User space mutex implementation.
 *
 * @file mutex.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_MUTEX_HPP
#define FIBERIZE_MUTEX_HPP

#include <mutex>

#include <fiberize/spinlock.hpp>
#include <fiberize/detail/lazydeque.hpp>

namespace fiberize {

namespace detail {

class Task;

} // namespace detail

/**
 * Implements a fully user space mutex, conforming to the Lockable concept.
 * @note This can be only used with threads managed by fiberize, as it uses the fiberize
 *       scheduler to suspend tasks, instead of calling out to the kernel.
 * @warning No thread must be waiting when the mutex is destroyed.
 */
class Mutex {
public:
    /**
     * Initialies the mutex.
     */
    Mutex(bool locked = false);

    /**
     * Locks the mutex.
     * @note When the task is waiting for the mutex it will process events.
     * @note In case of any exception, lock() will have no effect on the mutex.
     */
    void lock();

    /**
     * Tries to lock the mutex without blocking.
     */
    bool try_lock();

    /**
     * Unlocks the mutex.
     */
    void unlock();

private:
    /**
     * Unlocks the mutex, assuming we already hold the spinlock.
     */
    void unlock(std::unique_lock<Spinlock> lock);

    Spinlock spinlock;
    bool locked;
    std::atomic<uint64_t> released;
    uint64_t nextTicket;
    detail::LazyDeque<detail::Task*> queue;
};

} // namespace fiberize

#endif // FIBERIZE_MUTEX_HPP
