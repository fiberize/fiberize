#ifndef FIBERIZE_FUTURE_HPP
#define FIBERIZE_FUTURE_HPP

#include <fiberize/runnable.hpp>
#include <fiberize/eventenv.hpp>
#include <fiberize/promise.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {

namespace detail {

template <typename A>
struct CompletePromise {
    template <typename Closure>
    static void with(Promise<A>& promise, Closure&& closure) {
        promise.tryToComplete(closure());
    }
};

template <>
struct CompletePromise<void> {
    template <typename Closure>
    static void with(Promise<void>& promise, Closure&& closure) {
        closure();
        promise.tryToComplete();
    }
};

} // namespace detail

/**
 * A future is a fiber that returns a value.
 */
template <typename A>
class Future : public Runnable, public EventEnv {
public:
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    void operator () () override {
        auto controlBlock = context()->controlBlock();
        auto futureControlBlock = static_cast<detail::FutureControlBlock<A>*>(controlBlock);

        try {
            detail::CompletePromise<A>::with(futureControlBlock->result, [this] () {
                return run();
            });
        } catch (...) {
            futureControlBlock->result.tryToFail(std::current_exception());
        }
    }

    /**
     * Returns the reference to the current fiber.
     */
    FutureRef<A> self() const {
        if (self_.path() == devNullPath) {
            self_ = FutureRef<A>(std::make_shared<detail::LocalFutureRef<A>>(
                system(), static_cast<detail::FutureControlBlock<A>*>(Scheduler::current()->currentControlBlock())
            ));
        }
        return self_;
    }

private:
    mutable FutureRef<A> self_;
};

} // namespace fiberize

#endif // FIBERIZE_FUTURE_HPP
