#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/fibercontext.hpp>
#include <fiberize/event.hpp>
#include <fiberize/system.hpp>

namespace fiberize {
    
template <typename A>
struct Fiber: public detail::FiberBase {
    /**
     * Executes the fiber.
     */
    virtual A run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    virtual void _execute(std::shared_ptr<detail::ControlBlock> controlBlock) {
        FiberContext context(detail::Executor::current()->system, controlBlock);
        context.makeCurrent();

        self_ = FiberRef<A>(std::make_shared<detail::LocalFiberRef>(context.system, controlBlock));

        try {
            auto finished = Event<A>::fromPath(controlBlock->finishedEventPath);
            A result = run();
            std::lock_guard<std::mutex> lock(controlBlock->watchersMutex);
            for (AnyFiberRef& watcher : controlBlock->watchers)
                watcher.send(finished, result);
        } catch (...) {
            // TODO: proper logging
            std::cerr << "fiber crashed" << std::endl;
            std::lock_guard<std::mutex> lock(controlBlock->watchersMutex);
            auto crashed = Event<Unit>::fromPath(controlBlock->crashedEventPath);
            for (AnyFiberRef& watcher : controlBlock->watchers)
                watcher.send(crashed);
        }
    }
    
protected:

    /**
     * Returns the reference to the current fiber.
     */
    FiberRef<A> self() const {
        return self_;
    }

    /**
     * Returns the fiber system.
     */
    System* system() {
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
    FiberRef<A> self_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
