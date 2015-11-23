#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/eventenv.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/runnable.hpp>

namespace fiberize {

/**
 * A fiber is a lightweight thread.
 */
class Fiber : public Runnable, public EventEnv {
public:
    /**
     * Executes the fiber.
     */
    virtual void run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    void operator () () override;

    /**
     * Returns the reference to the current fiber.
     */
    FiberRef self() const;
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
