/**
 * Spinlock.
 *
 * @file spinlock.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_SPINLOCK_HPP
#define FIBERIZE_SPINLOCK_HPP

#include <atomic>

namespace fiberize {

/**
 * Spinlock implemented using atomic operations. Conforms to Lockable concept.
 */
class Spinlock {
public:
    Spinlock();
    Spinlock(const Spinlock&) = delete;
    Spinlock(Spinlock&&) = default;

    void lock();
    bool try_lock();
    void unlock();

private:
    std::atomic_flag locked;
};

} // namespace fiberize

#endif // FIBERIZE_SPINLOCK_HPP
