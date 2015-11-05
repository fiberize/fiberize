#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <unordered_map>
#include <list>

#include <boost/functional/hash.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>
#include <fiberize/handler.hpp>

namespace fiberize {
namespace detail {

struct HandlerBlock {
    std::list<std::unique_ptr<detail::Handler>> stackedHandlers;
};

class HandlerContext;
    
} // namespace detail

template <typename A>
class Event;

class Context {
public:
    /**
     * Creates a new context attached to the given mailbox.
     */
    Context(Mailbox* mailbox);
    
    /**
     * Destroys the context.
     */
    ~Context();
    
    /**
     * Yields control to the event loop.
     */
    void yield();
    
    /**
     * Handle an event in this context.
     */
    void handleEvent(const PendingEvent& event);

    /**
     * Sets up a handler for an event.
     */
    HandlerRef bind(const Path& path, detail::Handler* handler);
            
    /**
     * The current context.
     */
    static Context* current();
    
private:    
    Mailbox* mailbox_;
    
    // TODO: cache hashes
    std::unordered_map<Path, std::unique_ptr<detail::HandlerBlock>, boost::hash<Path>> handlerBlocks;

    /**
     * Fiber local context.
     */
    static thread_local Context* current_;
    
    friend detail::HandlerContext;
};

/**
 * Executes the next handler in the stack.
 */
void super();
    
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
