#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/buffer.hpp>
#include <fiberize/context.hpp>

namespace fiberize {
    
template <typename A>
class Fiber: public detail::FiberBase {
public:
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;
    
    /**
     * Executes the fiber and stores the result in a buffer.
     */
    virtual Buffer runStored() final {
        return Sendable<A, Local>::store(std::move(run()));
    }
    
    /**
     * Returns this fibers context.
     */
    Context* context() const {
        return Context::current;
    }
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
