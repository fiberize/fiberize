/**
 * User space condition variable implementation.
 *
 * @file condition.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/condition.hpp>
#include <fiberize/context.hpp>
#include <fiberize/detail/task.hpp>

namespace fiberize {

Condition::Condition()
    : released(0), nextTicket(1) {}

void Condition::await(std::unique_lock<Spinlock>& lock) {
    assert(lock.owns_lock());

    // Append ourself to the queue.
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
        // Something (probably a handler) threw an exception. Ensure that we don't eat the signal.
        lock.lock();
        uint64_t rel = released.load(std::memory_order_relaxed);
        if (rel < ticket) {
            // Set our task to nullptr to signal that we don't want the lock anymore.
            // We can calculate our index in the queue using the released and ticket numbers.
            uint64_t queueIndex = ticket - rel - 1;
            queue[size_t(queueIndex)] = nullptr;
        } else {
            // We already got signaled. Forward that signal to some other task.
            signal(lock);
        }
        lock.unlock();

        // Propagate the exception.
        throw;
    }

    lock.lock();
}

void Condition::signal(std::unique_lock<Spinlock>& lock) {
    assert(lock.owns_lock());

    // Try to wake up some task.
    while (!queue.empty()) {
        detail::Task* task = queue.front();
        queue.pop_front();
        std::atomic_fetch_add_explicit(&released, uint64_t(1), std::memory_order_release);

        // A task can be nullptr if it stopped waiting due to an exception.
        if (task != nullptr) {
            lock.unlock();
            context::detail::resume(task);
            lock.lock();
            return;
        }
    }
}

void Condition::signalAll(std::unique_lock<Spinlock>& lock) {
    assert(lock.owns_lock());

    while (!queue.empty()) {
        detail::Task* task = queue.front();
        queue.pop_front();
        std::atomic_fetch_add_explicit(&released, uint64_t(1), std::memory_order_release);

        // A task can be nullptr if it stopped waiting due to an exception.
        if (task != nullptr) {
            lock.unlock();
            context::detail::resume(task);
            lock.lock();
        }
    }
}

} // namespace fiberize
