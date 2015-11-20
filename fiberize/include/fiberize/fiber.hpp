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
    virtual void _execute(detail::ControlBlockPtr controlBlock) {
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
        if (self_.path() == Path(DevNullPath{})) {
            auto context = FiberContext::current();
            self_ = FiberRef<A>(std::make_shared<detail::LocalFiberRef>(context->system, context->controlBlock()));
        }
        return self_;
    }

    /**
     * Returns the fiber system.
     */
    FiberSystem* system() {
        return FiberContext::current()->system;
    }

    /**
     * Returns the context attached to this fiber.
     */
    FiberContext* context() {
        return FiberContext::current();
    }

    /**
     * Processes all pending events, then suspends and reschedules this fiber.
     */
    void yield() {
        context()->yield();
    }

    /**
     * Processes all pending events.
     */
    void process() {
        context()->process();
    }

    /**
     * Processes events in a loop, forever.
     */
    Void processForever() {
        return context()->processForever();
    }

    /**
     * Executes the next handler in a handler stack.
     */
    void super() {
        context()->super();
    }
    
private:
    mutable FiberRef<A> self_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
