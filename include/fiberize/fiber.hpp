#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>

namespace fiberize {
    
template <typename A>
class Fiber: public detail::FiberBase {
public:
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;
    
    virtual void entryPoint() {
        try {
            run();
        } catch (...) {
            
        }
        detail::Executor::current->currentControlBlock()->finished = true;
        detail::Executor::current->suspend();
    }
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
