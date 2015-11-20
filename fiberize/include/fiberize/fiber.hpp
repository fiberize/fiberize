#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/event.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/fibercontext.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>

namespace fiberize {
    
template <typename A>
struct Fiber : public detail::FiberBase {
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    virtual void _execute() {
        detail::ControlBlock* controlBlock = detail::Executor::current()->currentControlBlock();
        FiberContext context(detail::Executor::current()->system, controlBlock);
        context.makeCurrent();

        auto result = static_cast<Promise<A>*>(controlBlock->result.get());
        try {
            result->tryToComplete(run());
        } catch (...) {
            result->tryToFail(std::current_exception());
        }
    }
    
protected:

    /**
     * Returns the reference to the current fiber.
     */
    FiberRef<A> self() const {
        return FiberRef<A>(context()->fiberRef().impl());
    }

    /**
     * Returns the fiber system.
     */
    FiberSystem* system() const {
        return FiberContext::current()->system;
    }

    /**
     * Returns the context attached to this fiber.
     */
    FiberContext* context() const {
        return FiberContext::current();
    }

    /**
     * Processes all pending events, then suspends and reschedules this fiber.
     */
    void yield() const {
        context()->yield();
    }

    /**
     * Processes all pending events.
     */
    void process() const {
        context()->process();
    }

    /**
     * Processes events in a loop, forever.
     */
    Void processForever() const {
        return context()->processForever();
    }

    /**
     * Executes the next handler in a handler stack.
     */
    void super() const {
        context()->super();
    }
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
