#ifndef FIBERIZE_EVENTCONTEXT_HPP
#define FIBERIZE_EVENTCONTEXT_HPP

#include <unordered_map>
#include <list>

#include <boost/functional/hash.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>
#include <fiberize/handler.hpp>
#include <fiberize/fiberref.hpp>

namespace fiberize {

class FiberSystem;
class FiberContext;

namespace detail {

using HandlerBlock = std::vector<std::unique_ptr<detail::Handler>>;

class ControlBlock;

} // namespace detail

class EventContext {
public:
    /**
     * Creates a new context attached to the given control block.
     */
    EventContext(FiberSystem* system, detail::ControlBlock* controlBlock);
    
    /**
     * Processes all pending events and returns.
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
     * Handle an event in this context.
     */
    void handleEvent(const PendingEvent& event);

    /**
     * Sets up a handler for an event.
     */
    HandlerRef bind(const Path& path, std::unique_ptr<detail::Handler> handler);

    /**
     * The system this context is attached to.
     */
    FiberSystem* const system;

    /**
     * The control block of this fiber.
     */
    detail::ControlBlock* controlBlock();

    /**
     * The current fiber reference.
     */
    FiberRef fiberRef();

    /**
     * Makes this context a current context.
     *
     * You shoudn't really call this function, but I guess it might be useful if you
     * are making something really hacky.
     */
    void makeCurrent();

    /**
     * Returns the current event context.
     *
     * Calling it from a thread that is not fiberized is undefined behaviour.
     */
    static EventContext* current();

private:
    void collectGarbage(fiberize::detail::HandlerBlock& block);

    static thread_local EventContext* current_;

    std::unordered_map<Path, detail::HandlerBlock, boost::hash<Path>> handlerBlocks;
    detail::ControlBlock* controlBlock_;
    FiberRef fiberRef_;
};

    
} // namespace fiberize

#endif // FIBERIZE_EVENTCONTEXT_HPP
