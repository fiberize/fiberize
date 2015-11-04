#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>

namespace fiberize {
    
template <typename A>
class Fiber: public detail::FiberBase {
public:
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;
    
    virtual void entryPoint(detail::ControlBlock* controlBlock) {
        try {
            run();
        } catch (...) {
            
        }
    }
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
