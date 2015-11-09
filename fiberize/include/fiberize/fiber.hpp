#ifndef FIBERIZE_FIBER_HPP
#define FIBERIZE_FIBER_HPP

#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/context.hpp>
#include <fiberize/event.hpp>
#include <fiberize/system.hpp>

namespace fiberize {
    
template <typename A>
struct Fiber: public detail::FiberBase {
    Fiber() : context_(nullptr) {}

    /**
     * Executes the fiber.
     */
    virtual A run() = 0;

    /**
     * Called internally to start the fiber and take care of the result value and exceptions.
     */
    virtual void _execute(std::shared_ptr<detail::ControlBlock> controlBlock) {
        Context scopedContext(controlBlock, controlBlock->executor->system);
        context_ = &scopedContext;
        self_ = FiberRef(std::make_shared<detail::LocalFiberRef>(controlBlock->executor->system, controlBlock));

        try {
            auto finished = Event<A>::fromPath(controlBlock->finishedEventPath);
            A result = run();
            for (FiberRef& watcher : controlBlock->watchers)
                watcher.emit(finished, result);
        } catch (...) {
            // TODO: proper logging
            std::cerr << "fiber crashed" << std::endl;
            auto crashed = Event<Unit>::fromPath(controlBlock->crashedEventPath);
            for (FiberRef& watcher : controlBlock->watchers)
                watcher.emit(crashed);
        }
    }
    
protected:

    /**
     * Returns the reference to the current fiber.
     */
    FiberRef self() const {
        return self_;
    }

    /**
     * Returns the fiber system.
     */
    System* system() {
        return context_->system;
    }

    /**
     * Returns the context attached to this fiber.
     */
    Context* context() {
        return context_;
    }

    /**
     * Yields control to the event loop.
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
     * Executes the next handler in a handler stack.
     */
    void super() {
        context()->super();
    }

    /**
     * Awaits on the giben awaitable using this fiber's context.
     */
    template <typename Awaitable>
    auto await(Awaitable& awaitable) {
        return awaitable.await(context());
    }
    
private:
    Context* context_;
    FiberRef self_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBER_HPP
