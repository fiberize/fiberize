#include <fiberize/context.hpp>
#include <fiberize/detail/executor.hpp>

namespace fiberize {

Context::Context(Mailbox* mailbox): mailbox_(mailbox), currentBlock(nullptr), currentValue(nullptr) {
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
                event.buffer.free();
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
    Context* context = Context::current();
    assert(context->currentBlock != nullptr);
    
    /**
     * Check if we executed all handlers.
     */
    if (context->currentHandler == context->currentBlock->handlers.begin()) {
        return;
    }
    
    /**
     * Otherwise find an alive handler.
     */
    auto it = --context->currentHandler;
    while ((*it)->refCount == 0) {
        if (it == context->currentBlock->handlers.begin()) {
            /**
             * That was the last handler.
             */
            context->currentBlock->handlers.pop_front();
            return;
        } else {
            /**
             * Remove the handler and try to find another one.
             */
            auto copy = it;
            --it;
            context->currentBlock->handlers.erase(copy);
        }
    }
    
    /**
     * Set the current handler and execute it.
     */
    context->currentHandler = it;
    (*it)->handle(context->currentValue);
}

void Context::handleEvent(const PendingEvent& event) {
    /**
     * Find a handler block.
     */
    auto it = handlerBlocks.find(event.name);
    if (it == handlerBlocks.end())
        return;    
    currentBlock = it->second;
    
    /**
     * Get rid of dead handlers at the back.
     */
    while (!currentBlock->handlers.empty() && currentBlock->handlers.back()->refCount == 0) {
        delete currentBlock->handlers.back();
        currentBlock->handlers.pop_back();
    }
    
    /**
     * There are no handlers at all, remove the handler block.
     */
    if (currentBlock->handlers.empty()) {
        delete currentBlock;
        handlerBlocks.erase(it);
        currentBlock = nullptr;
        return;
    }
    
    /**
     * Deserialize the event value.
     */
    currentValue = malloc(currentBlock->objectSize);
    try {
        currentBlock->restore(event.buffer, currentValue);
    } catch (...) {
        currentBlock = nullptr;
        free(currentValue);
        currentValue = nullptr;
        throw;
    }
    
    /**
     * Execute the handler.
     */
    currentHandler = currentBlock->handlers.end();
    try {
        super();
    } catch (...) {
        currentBlock->destroy(currentValue);
        currentBlock = nullptr;
        free(currentValue);
        currentValue = nullptr;
        throw;
    }
    
    /**
     * Free the value.
     */
    currentBlock->destroy(currentValue);
    currentBlock = nullptr;
    free(currentValue);
    currentValue = nullptr;
}

Context* Context::current() {
    return current_;
}
 
thread_local Context* Context::current_ = nullptr;
 
}
