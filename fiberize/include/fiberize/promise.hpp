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

namespace fiberize {
namespace detail {

template <typename A>
class Success;

template <typename A>
class Failure;

/**
 * Quick and dirty result class. It would be better if it didn't use virtual tables.
 * TODO: use mach7 or whatever C++ implementation of pattern matching, or just
 * write a nice low level implementation.
 */
template <typename A>
class Result {
public:
    virtual ~Result() {};

    virtual bool isSuccess() = 0;
    virtual bool isFailure() = 0;

    virtual Success<A>* asSuccess() = 0;
    virtual Failure<A>* asFailure() = 0;
};

template <typename A>
class Success : public Result<A> {
public:
    template <typename... Args>
    Success(Args&&... args) : value(std::forward<Args>(args)...) {};

    bool isSuccess() override { return true; }
    bool isFailure() override { return false; }

    Success<A>* asSuccess() override { return this; }
    Failure<A>* asFailure() override { return nullptr; }

    A value;
};

template <>
class Success<void> : public Result<void> {
public:
    Success() {};

    bool isSuccess() override { return true; }
    bool isFailure() override { return false; }

    Success<void>* asSuccess() override { return this; }
    Failure<void>* asFailure() override { return nullptr; }
};

template <typename A>
class Failure : public Result<A> {
public:
    Failure(std::exception_ptr exception) : exception(exception) {};

    bool isSuccess() override { return false; }
    bool isFailure() override { return true; }

    Success<A>* asSuccess() override { return nullptr; }
    Failure<A>* asFailure() override { return this; }

    std::exception_ptr exception;
};

} // namespace detail

/**
 * Local implementation of a promise, using a mutex. This will likely have to change
 * in the future to support remote promises.
 */
template <typename A>
class Promise {
public:
    Promise() = default;
    explicit Promise(const Event<A>& watched) {
        handler = watched.bind([this] (const A& value) {
            tryToComplete(value);
            handler.release();
        });
    }
    Promise(const Promise&) = delete;
    Promise(Promise&&) = default;

    Promise& operator = (const Promise&) = delete;
    Promise& operator = (Promise&&) = delete;

    /**
     * Tries to complete the promise.
     */
    template <typename... Args>
    bool tryToComplete(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!result) {
            try {
                result.reset(new detail::Success<A>(std::forward<Args>(args)...));
            } catch (...) {
                result.reset(new detail::Failure<A>(std::current_exception()));
            }
            wakup();
            return true;
        } else {
            return false;
        }
    }
    
    /**
     * Tries to fail this promise.
     */
    bool tryToFail(std::exception_ptr error) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!result) {
            result.reset(new detail::Failure<A>(error));
            wakup();
            return true;
        } else {
            return false;
        }
    }
    
    /**
     * Awaits until the promise is completed or failed. If the promise
     * failed it will rethrow the exception.
     * TODO: optimize
     */
    A await() {
        std::unique_lock<std::mutex> lock(mutex);
        if (result) {
            detail::Success<A>* success = result->asSuccess();
            if (success != nullptr) {
                return success->value;
            } else {
                std::rethrow_exception(result->asFailure()->exception);
            }
        } else {
            waiting.push_back(context::self());
            lock.unlock();
            condition.await();
            return await();
        }
    }
    
private:
    void wakup() {
        for (FiberRef& ref : waiting)
            ref.send(condition);
        waiting = std::vector<FiberRef>();
    }

    std::mutex mutex;
    std::vector<FiberRef> waiting;
    std::unique_ptr<detail::Result<A>> result;
    Event<void> condition;
    HandlerRef handler;
};

template <>
class Promise<void> {
public:
    Promise() = default;
    explicit Promise(const Event<void>& watched) {
        handler = watched.bind([this] () {
            tryToComplete();
            handler.release();
        });
    }
    Promise(const Promise&) = delete;
    Promise(Promise&&) = default;

    Promise& operator = (const Promise&) = delete;
    Promise& operator = (Promise&&) = delete;

    /**
     * Tries to complete the promise.
     */
    bool tryToComplete() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!result) {
            try {
                result.reset(new detail::Success<void>());
            } catch (...) {
                result.reset(new detail::Failure<void>(std::current_exception()));
            }
            wakup();
            return true;
        } else {
            return false;
        }
    }

    /**
     * Tries to fail this promise.
     */
    bool tryToFail(std::exception_ptr error) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!result) {
            result.reset(new detail::Failure<void>(error));
            wakup();
            return true;
        } else {
            return false;
        }
    }

    /**
     * Awaits until the promise is completed or failed. If the promise
     * failed it will rethrow the exception.
     * TODO: optimize
     */
    void await() {
        std::unique_lock<std::mutex> lock(mutex);
        if (result) {
            detail::Success<void>* success = result->asSuccess();
            if (success != nullptr) {
                return;
            } else {
                std::rethrow_exception(result->asFailure()->exception);
            }
        } else {
            waiting.push_back(context::self());
            lock.unlock();
            condition.await();
            return await();
        }
    }

private:
    void wakup() {
        for (FiberRef& ref : waiting)
            ref.send(condition);
        waiting = std::vector<FiberRef>();
    }

    std::mutex mutex;
    std::vector<FiberRef> waiting;
    std::unique_ptr<detail::Result<void>> result;
    Event<void> condition;
    HandlerRef handler;
};

} // namespace fiberize

#endif // FIBERIZE_PROMISE_HPP

