#include <fiberize/eventcontext.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {

EventContext::EventContext(FiberSystem* system, detail::ControlBlock* controlBlock)
    : system(system), controlBlock_(controlBlock) {}

void EventContext::process()
{
    PendingEvent event;
    boost::unique_lock<detail::ControlBlockMutex> lock(controlBlock_->mutex);
    while (controlBlock_->mailbox->dequeue(event)) {
        lock.unlock();
        try {
            handleEvent(event);
        } catch (...) {
            event.freeData(event.data);
            throw;
        }
        event.freeData(event.data);
        lock.lock();
    }
}

void EventContext::processForever()
{
    PendingEvent event;
    while (true) {
        /**
         * First, process all pending events.
         */
        PendingEvent event;
        boost::unique_lock<detail::ControlBlockMutex> lock(controlBlock_->mutex);
        while (controlBlock_->mailbox->dequeue(event)) {
            lock.unlock();
            try {
                handleEvent(event);
            } catch (...) {
                event.freeData(event.data);
                throw;
            }
            event.freeData(event.data);
            lock.lock();
        }

        /**
         * Prepare to suspend the thread by locking the control block.
         */

        /**
         * No more events, we can suspend the thread.
         */
        Scheduler::current()->suspend(std::move(lock));
    }
}

void EventContext::super() {
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

void EventContext::handleEvent(const PendingEvent& event) {
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

HandlerRef EventContext::bind(const Path& path, fiberize::detail::Handler* handler) {
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

detail::ControlBlock* EventContext::controlBlock() {
    return controlBlock_;
}

void EventContext::makeCurrent() {
    current_ = this;
}

EventContext* EventContext::current() {
    return current_;
}

FiberRef EventContext::fiberRef() {
    if (fiberRef_.path() == Path(DevNullPath{})) {
        fiberRef_ = FiberRef(std::make_shared<detail::LocalFiberRef>(system, controlBlock_));
    }
    return fiberRef_;
}

thread_local EventContext* EventContext::current_ = nullptr;
 
}
