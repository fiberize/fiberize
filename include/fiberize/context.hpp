#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <unordered_map>
#include <list>

#include <boost/functional/hash.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>
#include <fiberize/handler.hpp>

namespace fiberize {

class System;

namespace detail {

struct HandlerBlock {
    std::list<std::unique_ptr<detail::Handler>> stackedHandlers;
};

class HandlerContext;
struct ControlBlock;

} // namespace detail

template <typename A>
class Event;

class Context {
public:
    /**
     * Creates a new context attached to the given control block.
     */
    Context(detail::ControlBlock* controlBlock, System* system);
    
    /**
     * Destroys the context.
     */
    ~Context();
    
    /**
     * Yields control to the event loop.
     */
    void yield();
    
    /**
     * Processes all pending events.
     */
    void process();

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
    System* const system;

    /**
     * The control block of this fiber.
     */
    detail::ControlBlock* const controlBlock;
    
private:
    // TODO: cache hashes
    std::unordered_map<Path, std::unique_ptr<detail::HandlerBlock>, boost::hash<Path>> handlerBlocks;
    std::unique_ptr<detail::HandlerContext> handlerContext;
    
    friend detail::HandlerContext;
};
    
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
