#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/context.hpp>
#include <fiberize/buffer.hpp>
#include <fiberize/event.hpp>

namespace fiberize {
    
template <typename A>
struct Fiber: public detail::FiberBase {
    typedef A result_type;
    
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    virtual void _execute() {
        auto controlBlock = detail::Executor::current()->currentControlBlock();
        try {
            auto finished = Event<A>::fromPath(controlBlock->finishedEventPath);
            controlBlock->parent.emit(finished, run());
        } catch (...) {
            auto crashed = Event<Unit>::fromPath(controlBlock->crashedEventPath);
            controlBlock->parent.emit(crashed);
        }
    }
    
protected:
    /**
     * Yields control to the event loop.
     */
    void yield() const {
        return Context::current()->yield();
    }
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
