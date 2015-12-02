/**
 * User space mutex implementation.
 *
 * @file mutex.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/mutex.hpp>
#include <fiberize/context.hpp>
#include <fiberize/detail/task.hpp>

#include <atomic>

namespace fiberize {

Mutex::Mutex(bool locked)
    : locked(locked), released(0), nextTicket(1) {}

void Mutex::lock() {
    std::unique_lock<Spinlock> lock(spinlock);

    // No contention path.
    if (!locked) {
        locked = true;
        return;
    }

    // Contention, we have to wait. append ourself to the queue.
    queue.push_back(context::detail::task());

    // Important: grab the ticket AFTER appending to the queue, to ensure that
    //            the ticket number is consistent if the queue throws an exception.
    uint64_t ticket = nextTicket++;

    lock.unlock();

    try {
        // Process events when we are waiting.
        while (released.load(std::memory_order_consume) < ticket) {
            context::process();
            context::detail::suspend();
        }
    } catch (...) {
        // Something (probably a handler) threw an exception. Ensure we don't take the lock.
        lock.lock();
        uint64_t rel = released.load(std::memory_order_relaxed);
        if (rel < ticket) {
            // Set our task to nullptr to signal that we don't want the lock anymore.
            // We can calculate our index in the queue using the released and ticket numbers.
            uint64_t queueIndex = ticket - rel - 1;
            queue[size_t(queueIndex)] = nullptr;
            lock.unlock();
        } else {
            unlock(std::move(lock));
        }

        // Propagate the exception.
        throw;
    }
}

bool Mutex::try_lock() {
    std::unique_lock<Spinlock> lock(spinlock);
    if (!locked) {
        locked = true;
        return true;
    } else {
        return false;
    }
}

void Mutex::unlock() {
    unlock(std::unique_lock<Spinlock>(spinlock));
}

void Mutex::unlock(std::unique_lock<Spinlock> lock) {
    assert(lock.owns_lock());

    // Try to wake up some task.
    while (!queue.empty()) {
        detail::Task* task = queue.front();
        queue.pop_front();
        std::atomic_fetch_add_explicit(&released, uint64_t(1), std::memory_order_release);

        // A task can be nullptr if it changed its mind about getting the lock.
        if (task != nullptr) {
            lock.unlock();
            context::detail::resume(task);
            return;
        }
    }

    // If we didn't find any task, unlock the mutex.
    locked = false;
}

} // namespace fiberize
