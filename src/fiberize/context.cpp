#include <fiberize/context.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/controlblock.hpp>

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

Context::Context(detail::ControlBlock* controlBlock, System* system): controlBlock(controlBlock), system(system) {
    Context::current_ = this;
    controlBlock->grab();
}

Context::~Context() {
    Context::current_ = nullptr;
    controlBlock->drop();
}

void Context::yield() {
    while (true) {
        process();
        
        // Suspend the current thread.
        detail::Executor* executor = detail::Executor::current();
        if (executor != nullptr) {
            controlBlock->mutex.lock();

            /**
             * It's possible that someone queued a message before we locked the mutex.
             * Check if this is the case.
             */
            PendingEvent event;
            if (controlBlock->mailbox->dequeue(event)) {
                /**
                 * Too bad, now we have to process it and start again.
                 */
                controlBlock->mutex.unlock();
                
                try {
                    handleEvent(event);
                } catch (...) {
                    event.freeData(event.data);
                    throw;
                }

                continue;
            }
            
            /**
             * No new events, we can suspend the thread.
             */
            Context::current_ = nullptr;
            executor->suspend();
            Context::current_ = this;
        } else {
            // TODO: change this to conditions.
            using namespace std::literals;
            std::this_thread::sleep_for(1ms);
        }
    }
}

void Context::process()
{
    PendingEvent event;
    while (controlBlock->mailbox->dequeue(event)) {
        try {
            handleEvent(event);
        } catch (...) {
            event.freeData(event.data);
            throw;
        }
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
