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
    const bool condition = false;
    processUntil(condition);

    /**
     * Obviously, the condition will always be false.
     */
    __builtin_unreachable();
}

void EventContext::processUntil(const bool& condition)
{
    PendingEvent event;
    while (!condition) {
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

            /**
             * Short-circuit when condition is triggered.
             */
            if (condition) {
                return;
            }

            lock.lock();
        }

        /**
         * No more events, we can suspend the thread.
         */
        Scheduler::current()->suspend(std::move(lock));
    }
}

void EventContext::handleEvent(const PendingEvent& event) {
    /**
     * Find a handler block.
     */
    auto blockIt = handlerBlocks.find(event.path);
    if (blockIt == handlerBlocks.end())
        return;
    detail::HandlerBlock* block = blockIt->second.get();
    
    /**
     * GC dead handlers.
     */
    collectGarbage(block);

    /**
     * There are no alive handlers, remove the handler block.
     */
    if (block->handlers.empty()) {
        handlerBlocks.erase(blockIt);
        return;
    }
        
    /**
     * Execute the handlers.
     */
    auto it = block->handlers.rbegin();
    auto end = block->handlers.rend();
    while (it != end) {
        (*it)->execute(event.data);
        ++it;
    }
}

void EventContext::collectGarbage(detail::HandlerBlock* block) {
    size_t i = 0;

    const size_t n = block->handlers.size();
    for (size_t j = 0; j < n; ++j) {
        if (!block->handlers[j]->isDestroyed()) {
            block->handlers[i].reset(block->handlers[j].release());
            ++i;
        }
    }

    block->handlers.resize(i);
}

HandlerRef EventContext::bind(const Path& path, std::unique_ptr<fiberize::detail::Handler> handler) {
    detail::HandlerBlock* block;
    auto it = handlerBlocks.find(path);
    if (it == handlerBlocks.end()) {
        block = new detail::HandlerBlock;
        handlerBlocks.emplace(std::make_pair(path, std::unique_ptr<detail::HandlerBlock>(block)));
    } else {
        block = it->second.get();
    }

    HandlerRef ref(handler.get());
    block->handlers.emplace_back(std::move(handler));
    return ref;
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
    if (fiberRef_.path() == devNullPath) {
        fiberRef_ = FiberRef(std::make_shared<detail::LocalFiberRef>(system, controlBlock_));
    }
    return fiberRef_;
}

thread_local EventContext* EventContext::current_ = nullptr;
 
}