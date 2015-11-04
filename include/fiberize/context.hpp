#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <map>
#include <list>

#include <fiberize/mailbox.hpp>

namespace fiberize {
namespace detail {
    
struct Handler {
    uint64_t refCount;
    std::function<void (void*)> handle;
};

struct HandlerBlock {
    uint64_t objectSize;
    void (*restore)(Buffer buffer, void* output);
    void (*destroy)(void* value);
    
    std::list<Handler*> handlers;
};

} // namespace detail

class HandlerRef {
public:
    HandlerRef(detail::Handler* handler): handler(handler) {
        if (handler != nullptr)
            handler->refCount += 1;
    }
    
    HandlerRef(const HandlerRef& ref): handler(handler) {
        if (handler != nullptr)
            handler->refCount += 1;
    }
    
    HandlerRef(HandlerRef&& ref): handler(handler) {
        ref.handler = nullptr;
    }
    
    ~HandlerRef() {
        release();
    }
    
    HandlerRef& operator = (const HandlerRef& ref) {
        if (handler != nullptr)
            handler->refCount -= 1;
        handler = ref.handler;
        if (handler != nullptr)
            handler->refCount += 1;
        return *this;
    }
    
    HandlerRef& operator = (HandlerRef&& ref) {
        if (handler != nullptr)
            handler->refCount -= 1;
        handler = ref.handler;
        ref.handler = nullptr;
        return *this;
    }
    
    void release() {
        if (handler != nullptr) {
            handler->refCount -= 1;
            handler = nullptr;
        }
    }
    
private:
    detail::Handler* handler;
};

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
     * Binds an event to a handler.
     */
    template <typename A>
    HandlerRef bind(const Event<A>& event, const std::function<void (const A&)>& handle) {
        detail::HandlerBlock* block;
        
        auto it = handlerBlocks.find(event.name());
        if (it == handlerBlocks.end()) {
            block = new detail::HandlerBlock;
            block->objectSize = sizeof(A);
            block->restore = Sendable<A>::restore;
            block->destroy = Sendable<A>::destroy;
        } else {
            block = it->second;
        }

        detail::Handler* handler;
        handler->refCount = 0;
        handler->handle = handle;
        block->handlers.push_back(handler);
        
        return HandlerRef(handler);
    }
    
    /**
     * The current context.
     */
    static Context* current();
    
private:
    Mailbox* mailbox_;
    
    // Handler context.
    detail::HandlerBlock* currentBlock;
    std::list<detail::Handler*>::iterator currentHandler;
    void* currentValue;

    // TODO: make this a hash map, use cached hashes
    std::map<std::string, detail::HandlerBlock*> handlerBlocks;

    /**
     * Fiber local context.
     */
    static thread_local Context* current_;
    
    friend void super();
};

/**
 * Executes the superhandler.
 */
void super();
    
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
