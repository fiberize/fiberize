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

namespace fiberize {

namespace detail {

template <typename A>
struct Box {
    template <typename... Args>
    Box(Args&&... args) : value(std::forward<Args>(args)...) {}

    A get() const {
        return value;
    }

    A value;
};

template <>
struct Box<void> {
    void get() const {}
};

} // namespace detail

/**
 * A promise that will contain a value of type A.
 *
 * @warning The copy constructor of A must be thread-safe.
 */
template <typename A>
class Promise {
private:
    enum State : uint8_t {
        Empty,
        Completing,
        Complete
    };

public:
    /**
     * Creates an empty promise.
     */
    Promise() : isCompleted(false) {}
    Promise(const Promise&) = delete;
    Promise(Promise&&) = default;

    /**
     * Creates a promise that will watch the given event and complete when it fires.
     */
    Promise(const Event<A>& watched) : isCompleted(false) {
        handler = watched.bind([this] (const A& value) {
            complete(value);
        });
    }

    /**
     * Destroys the promise.
     */
    ~Promise() {
        if (isCompleted) {
            result.~Box();
        }
    }

    /**
     * Tries to complete the promise with a value.
     */
    template <typename... Args>
    bool complete(Args&&... args) {
        std::unique_lock<std::mutex> lock(mutex);

        /**
         * Check if the promise was empty.
         */
        if (isCompleted)
            return false;

        /**
         * Try to construct the result.
         */
        try {
            new (&result) detail::Box<A>{std::forward<Args>(args)...};
            isCompleted = true;
        } catch (...) {
            // We failed to complete the promise.
            return false;
        }

        /**
         * Wake up tasks awaiting this promise.
         */
        for (FiberRef& ref : awaiting) {
            ref.send(completed);
        }
        awaiting.clear();

        return true;
    }

    /**
     * Awaits
     */
    A await() {
        std::unique_lock<std::mutex> lock(mutex);

        /**
         * If the promise is empty we have to sleep until it is completed.
         */
        if (!isCompleted) {
            /**
             * Add this task to the awaiting list and wait for the result.
             */
            awaiting.push_back(context::self());
            lock.unlock();
            completed.await();
            lock.lock();
        }

        return result.get();
    }

private:
    HandlerRef handler;
    Event<void> completed;
    std::mutex mutex;
    std::vector<FiberRef> awaiting;
    bool isCompleted;

    union {
        detail::Box<A> result;
    };
};

} // namespace fiberize

#endif // FIBERIZE_PROMISE_HPP

