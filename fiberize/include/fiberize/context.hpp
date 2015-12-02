/**
 * Global functions available in fibers.
 *
 * @file context.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <mutex>

#include <fiberize/fiberref.hpp>
#include <fiberize/spinlock.hpp>

namespace fiberize {

class FiberSystem;
class Scheduler;

namespace detail {

class Task;

} // namespace detail

namespace context {

/**
 * @defgroup context Context
 *
 * Global functions available inside any task.
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
 * Returns a thread local random generator.
 */
std::mt19937_64& random();

/**
 * Suspends and reschedules this fiber, allowing other fibers to be run.
 */
void yield();

/**
 * Stop the actor.
 */
void stop();

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
 * Processes all pending events (or until the actor is stopped).
 */
void process(std::unique_lock<Spinlock>& lock);

/**
 * Returns the currently running task.
 */
fiberize::detail::Task* task();

/**
 * Dispatches an event to the handlers.
 */
void dispatchEvent(const PendingEvent& event);

/**
 * Binds an event with the given path to a handler.
 */
HandlerRef bind(const Path& path, std::unique_ptr<fiberize::detail::Handler> handler);

/**
 * Suspends execution of this task.
 */
void suspend();

/**
 * Resumes execution of a suspended task.
 */
void resume(fiberize::detail::Task* task);

/**
 * Resumes execution of a suspended task.
 */
void resume(fiberize::detail::Task* task, std::unique_lock<Spinlock> lock);

} // namespace detail

///@}

} // namespace context
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
