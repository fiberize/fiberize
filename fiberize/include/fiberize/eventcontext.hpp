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

struct HandlerBlock {
    std::list<std::unique_ptr<detail::Handler>> stackedHandlers;
};

class HandlerContext {
public:
    explicit inline HandlerContext(HandlerBlock* handlerBlock, const void* data)
        : handlerBlock(handlerBlock)
        , handler(handlerBlock->stackedHandlers.end())
        , data(data) {
    };

    HandlerBlock* handlerBlock;
    std::list<std::unique_ptr<Handler>>::iterator handler;
    const void* data;
};

struct ControlBlock;

} // namespace detail

template <typename A>
class Event;

class EventContext {
public:
    /**
     * Creates a new context attached to the given control block.
     */
    EventContext(FiberSystem* system, detail::ControlBlock* controlBlock);
    
    /**
     * Processes all pending events.
     */
    void process();

    /**
     * Processes events in a loop, forever.
     */
    [[ noreturn ]] void processForever();

    /**
     * Executes the next handler in a stack.
     */
    void super();
    
    /**
     * Handle an event in this context.
     */
    void handleEvent(const PendingEvent& event);

    /**
     * Sets up a handler for an event.
     */
    HandlerRef bind(const Path& path, detail::Handler* handler);

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
    static thread_local EventContext* current_;

    // TODO: cache hashes
    std::unordered_map<Path, std::unique_ptr<detail::HandlerBlock>, boost::hash<Path>> handlerBlocks;
    std::unique_ptr<detail::HandlerContext> handlerContext;
    detail::ControlBlock* controlBlock_;
    FiberRef fiberRef_;
    
    friend detail::HandlerContext;
};

    
} // namespace fiberize

#endif // FIBERIZE_EVENTCONTEXT_HPP
