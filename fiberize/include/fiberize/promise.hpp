#ifndef FIBERIZE_PROMISE_HPP
#define FIBERIZE_PROMISE_HPP

#include <mutex>
#include <vector>
#include <exception>

#include <boost/variant.hpp>

#include <fiberize/event.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/event-inl.hpp>
#include <fiberize/context.hpp>
#include <fiberize/result.hpp>
#include <fiberize/mutex.hpp>
#include <fiberize/condition.hpp>

namespace fiberize {

namespace detail {

template <typename A>
struct Box {
    template <typename... Args>
    Box(Args&&... args) : value(std::forward<Args>(args)...) {}

    A copy() const {
        return value;
    }

    A value;
};

template <>
struct Box<void> {
    void copy() const {}
};

} // namespace detail

/**
 * A promise that will contain a value of type A.
 */
template <typename A>
class Promise {
public:
    /**
     * Creates an empty promise.
     */
    Promise() {}
    Promise(const Promise&) = delete;
    Promise(Promise&&) = default;

    /**
     * Creates a promise that will watch the given event and complete when it fires.
     */
    Promise(const Event<A>& watched) {
        handler = watched.bind([this] (const A& value) {
            complete(value);
        });
    }

    /**
     * Tries to complete the promise with a value.
     */
    template <typename... Args>
    bool complete(Args&&... args) {
        std::unique_lock<Spinlock> lock(spinlock);

        /**
         * Check if the promise was empty.
         */
        if (result)
            return false;

        /**
         * Try to construct the result.
         */
        try {
            result.emplace(std::forward<Args>(args)...);
        } catch (...) {
            // We failed to complete the promise.
            return false;
        }

        /**
         * Wake up tasks awaiting this promise.
         */
        completed.signalAll(lock);

        return true;
    }

    /**
     * Awaits until the promise is complete.
     */
    A await() {
        std::unique_lock<Spinlock> lock(spinlock);
        if (!result) {
            completed.await(lock);
        }
        return result.get().copy();
    }

private:
    HandlerRef handler;
    Condition completed;
    Spinlock spinlock;
    boost::optional<detail::Box<A>> result;
};

} // namespace fiberize

#endif // FIBERIZE_PROMISE_HPP

