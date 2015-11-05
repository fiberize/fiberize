#include <fiberize/context.hpp>
#include <fiberize/detail/executor.hpp>

namespace fiberize {
namespace detail {

class HandlerContext {
public:
    HandlerContext(HandlerBlock* handlerBlock, const void* data)
        : handlerBlock(handlerBlock)
        , handler(handlerBlock->stackedHandlers.end())
        , data(data) {
        stashedContext = Context::current_;
        Context::current_ = nullptr;
        HandlerContext::current_ = this;
    };
    
    ~HandlerContext() {
        Context::current_ = stashedContext;
        HandlerContext::current_ = nullptr;
    }
    
    const void* data;
    HandlerBlock* handlerBlock;
    std::list<std::unique_ptr<Handler>>::iterator handler;
    
    static HandlerContext* current() {
        return current_;
    }
    
private:
    static thread_local HandlerContext* current_;

    Context* stashedContext;
};
    
thread_local HandlerContext* HandlerContext::current_ = nullptr;
    
} // namespace detail

Context::Context(Mailbox* mailbox): mailbox_(mailbox) {
    Context::current_ = this;
    mailbox_->grab();
}

Context::~Context() {
    Context::current_ = nullptr;
    mailbox_->drop();
}

void Context::yield() {
    PendingEvent event;
    while (true) {
        while (mailbox_->dequeue(event)) {
            try {
                handleEvent(event);
            } catch (...) {
                event.freeData(event.data);
                throw;
            }
        }
        
        // Suspend the current thread.
        Context::current_ = nullptr;
        detail::Executor* executor = detail::Executor::current;
        if (executor != nullptr) {
            detail::Executor::current->suspend();
        } else {
            // TODO: change this to conditions.
            using namespace std::literals;
            std::this_thread::sleep_for(1ms);
        }
        Context::current_ = this;
    }
}

void super() {
    detail::HandlerContext* context = detail::HandlerContext::current();
    
    /**
     * Check if we executed all handlers.
     */
    if (context->handler == context->handlerBlock->stackedHandlers.begin()) {
        return;
    }
    
    /**
     * Otherwise find an alive handler.
     */
    auto it = --context->handler;
    while ((*it)->isDestroyed()) {
        if (it == context->handlerBlock->stackedHandlers.begin()) {
            /**
             * That was the last handler.
             */
            context->handlerBlock->stackedHandlers.pop_front();
            return;
        } else {
            /**
             * Remove the handler and try to find another one.
             */
            auto copy = it;
            --it;
            context->handlerBlock->stackedHandlers.erase(copy);
        }
    }
    
    /**
     * Set the current handler and execute it.
     */
    context->handler = it;
    (*it)->execute(context->data);
}

void Context::handleEvent(const PendingEvent& event) {
    /**
     * Find a handler block.
     */
    auto it = handlerBlocks.find(event.path);
    if (it == handlerBlocks.end())
        return;
    detail::HandlerBlock* block = it->second.get();
    
    /**
     * Get rid of dead handlers at the back.
     */
    while (!block->stackedHandlers.empty() && block->stackedHandlers.back()->isDestroyed()) {
        block->stackedHandlers.pop_back();
    }
    
    /**
     * There are no handlers at all, remove the handler block.
     */
    if (block->stackedHandlers.empty()) {
        handlerBlocks.erase(it);
        return;
    }
        
    /**
     * Execute the handler.
     */
    detail::HandlerContext handlerContext(block, event.data);
    super();
}

HandlerRef Context::bind(const Path& path, fiberize::detail::Handler* handler) {
    detail::HandlerBlock* block;
    auto it = handlerBlocks.find(path);
    if (it == handlerBlocks.end()) {
        block = new detail::HandlerBlock;
        handlerBlocks.emplace(std::make_pair(path, std::unique_ptr<detail::HandlerBlock>(block)));
    } else {
        block = it->second.get();
    }

    block->stackedHandlers.emplace_back(handler);        
    return HandlerRef(handler);
}

Context* Context::current() {
    return current_;
}
 
thread_local Context* Context::current_ = nullptr;
 
}
