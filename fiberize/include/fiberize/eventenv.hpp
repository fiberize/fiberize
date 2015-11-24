#ifndef FIBERIZE_EVENTENV_HPP
#define FIBERIZE_EVENTENV_HPP

namespace fiberize {

class FiberSystem;
class EventContext;

/**
 * A mixin with some helper functions for event processing.
 */
class EventEnv {
public:
    /**
     * Returns the fiber system.
     */
    FiberSystem* system() const;

    /**
     * Returns the context attached to this fiber.
     */
    EventContext* context() const;

    /**
     * Suspends and reschedules this fiber, allowing other fibers to be run.
     */
    void yield() const;

    /**
     * Processes all pending events.
     */
    void process() const;

    /**
     * Processes events in a loop, forever.
     */
    [[ noreturn ]] void processForever() const;

    /**
     * Processes events until the condition is true.
     */
    void processUntil(const bool& condition) const;
};

} // namespace fiberize

#endif // FIBERIZE_EVENTENV_HPP
