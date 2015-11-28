#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <fiberize/fiberref.hpp>

namespace fiberize {

class FiberSystem;
class Scheduler;

namespace context {

/**
 * @defgroup context Fiber context
 *
 * Global methods available inside any fiberized thread or fiber.
 *
 */
///@{

/**
 * Returns the fiber system.
 */
FiberSystem* system();

/**
 * Scheduler executing this fiber.
 */
Scheduler* scheduler();

/**
 * Suspends and reschedules this fiber, allowing other fibers to be run.
 */
void yield();

/**
 * Processes all pending events.
 */
void process();

/**
 * Processes events in a loop, forever.
 */
[[ noreturn ]] void processForever();

/**
 * Processes events until the condition is true.
 */
void processUntil(const bool& condition);

/**
 * Returns reference to the currently running fiber.
 */
FiberRef self();

///@}

} // namespace context
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
