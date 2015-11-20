#include <fiberize/fibercontext.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {

FiberContext::FiberContext(FiberSystem* system, detail::ControlBlockPtr controlBlock)
    : system(system), controlBlock_(std::move(controlBlock)), fiberRef_(std::make_shared<detail::LocalFiberRef>(system, controlBlock_)) {
    controlBlock_->fiberContext = this;
}

void FiberContext::yield() {
    process();

    // Suspend the current thread.
    detail::Executor* executor = detail::Executor::current();
    if (executor != nullptr) {
        boost::unique_lock<detail::ControlBlockMutex> lock(controlBlock_->mutex);

        /**
         * It's possible that someone queued a message before we locked the mutex.
         * Check if this is the case.
         */
        PendingEvent event;
        if (controlBlock_->mailbox->dequeue(event)) {
            /**
             * Too bad, now we have to process it and start again.
             */
            lock.unlock();

            try {
                handleEvent(event);
            } catch (...) {
                event.freeData(event.data);
                throw;
            }
            event.freeData(event.data);

            /**
             * Try again.
             */
            yield();
        } else {
            /**
            * No new events, we can suspend the thread.
            */
            executor->suspendAndReschedule(std::move(lock));
        }
    } else {
        pthread_yield();
    }
}

void FiberContext::process()
{
    PendingEvent event;
    while (controlBlock_->mailbox->dequeue(event)) {
        try {
            handleEvent(event);
        } catch (...) {
            event.freeData(event.data);
            throw;
        }
        event.freeData(event.data);
    }
}

Void FiberContext::processForever()
{
    while (true) {
        process();

        // Suspend the current thread.
        detail::Executor* executor = detail::Executor::current();
        if (executor != nullptr) {
            boost::unique_lock<detail::ControlBlockMutex> lock(controlBlock_->mutex);

            /**
             * It's possible that someone queued a message before we locked the mutex.
             * Check if this is the case.
             */
            PendingEvent event;
            if (controlBlock_->mailbox->dequeue(event)) {
                /**
                 * Too bad, now we have to process it and start again.
                 */
                lock.unlock();

                try {
                    handleEvent(event);
                } catch (...) {
                    event.freeData(event.data);
                    throw;
                }
                event.freeData(event.data);

                continue;
            }

            /**
             * No new events, we can suspend the thread.
             */
            executor->suspend(std::move(lock));
        } else {
            pthread_yield();
        }
    }
}

void FiberContext::super() {
    /**
     * Check if we executed all handlers.
     */
    if (handlerContext->handler == handlerContext->handlerBlock->stackedHandlers.begin()) {
        return;
    }
    
    /**
     * Otherwise find an alive handler.
     */
    auto it = --handlerContext->handler;
    while ((*it)->isDestroyed()) {
        if (it == handlerContext->handlerBlock->stackedHandlers.begin()) {
            /**
             * That was the last handler.
             */
            handlerContext->handlerBlock->stackedHandlers.pop_front();
            return;
        } else {
            /**
             * Remove the handler and try to find another one.
             */
            auto copy = it;
            --it;
            handlerContext->handlerBlock->stackedHandlers.erase(copy);
        }
    }
    
    /**
     * Set the current handler and execute it.
     */
    handlerContext->handler = it;
    (*it)->execute(handlerContext->data);
}

void FiberContext::handleEvent(const PendingEvent& event) {
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
    handlerContext.reset(new detail::HandlerContext(block, event.data));
    super();
    handlerContext.reset();
}

HandlerRef FiberContext::bind(const Path& path, fiberize::detail::Handler* handler) {
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

detail::ControlBlockPtr FiberContext::controlBlock() {
    return controlBlock_;
}

void FiberContext::makeCurrent() {
    current_ = this;
}

FiberContext* FiberContext::current() {
    return current_;
}

AnyFiberRef FiberContext::fiberRef() {
    return fiberRef_;
}

thread_local FiberContext* FiberContext::current_ = nullptr;
 
}
