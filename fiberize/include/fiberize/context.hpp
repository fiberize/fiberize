#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <fiberize/fiberref.hpp>

namespace fiberize {

class FiberSystem;
class Scheduler;

namespace detail {

class ControlBlock;

} // namespace detail

namespace context {

/**
 * @defgroup context Fiber context
 *
 * Global methods available inside any fiberized OS thread, fiber, actor, future or thread.
 *
 */
///@{

/**
 * Returns the fiber system.
 */
FiberSystem* system();

/**
 * Returns the scheduler executing this fiber.
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
 * @note This function performs a memory allocation.
 */
FiberRef self();

namespace detail {

/**
 * Returns the control block of the currently executing fiber.
 */
fiberize::detail::ControlBlock* controlBlock();

/**
 * Dispatches an event to the handlers.
 */
void dispatchEvent(const PendingEvent& event);

/**
 * Binds an event with the given path to a handler.
 */
HandlerRef bind(const Path& path, std::unique_ptr<fiberize::detail::Handler> handler);

/**
 * Resumes execution of a suspended fiber.
 */
void resume(fiberize::detail::ControlBlock* controlBlock, boost::unique_lock<boost::detail::spinlock> lock);

} // namespace detail

///@}

} // namespace context
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
