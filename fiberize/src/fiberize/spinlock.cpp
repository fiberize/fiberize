/**
 * Spinlock.
 *
 * @file spinlock.hpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/spinlock.hpp>

namespace fiberize {

Spinlock::Spinlock()
    : locked(ATOMIC_FLAG_INIT)
    {}

void Spinlock::lock() {
    while (!try_lock()) {
        // Spin.
    }
}

bool Spinlock::try_lock() {
    return !locked.test_and_set(std::memory_order_acquire);
}

void Spinlock::unlock() {
    locked.clear(std::memory_order_release);
}

} // namespace fiberize
