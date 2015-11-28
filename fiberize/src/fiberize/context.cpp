#include <fiberize/context.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/exceptions.hpp>
#include <fiberize/events.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {
namespace context {

FiberSystem* system() {
    return Scheduler::current()->system();
}

Scheduler* scheduler() {
    return Scheduler::current();
}

void yield() {
    boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(detail::controlBlock()->mutex);
    Scheduler::current()->yield(std::move(lock));
}

void process() {
    PendingEvent event;
    auto block = detail::controlBlock();
    boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(block->mutex);
    while (block->mailbox->dequeue(event)) {
        lock.unlock();
        try {
            detail::dispatchEvent(event);
        } catch (...) {
            if (event.freeData)
                event.freeData(event.data);
            throw;
        }
        if (event.freeData)
            event.freeData(event.data);
        lock.lock();
    }
}

void processForever() {
    const bool condition = false;
    processUntil(condition);

    /**
     * Obviously, the condition will always be false.
     */
    __builtin_unreachable();
}

static void initializeHandlers() {
    /**
     * This probably could be improved a lot.
     */
    auto block = detail::controlBlock();
    std::unique_ptr<fiberize::detail::Handler> killHandler(new fiberize::detail::TypedHandler<void>([] () {
        throw Killed();
    }));
    killHandler->grab();
    block->handlers[kill.path()].emplace_back(std::move(killHandler));
    block->handlersInitialized = true;
}

void processUntil(const bool& condition) {
    auto block = detail::controlBlock();
    PendingEvent event;
    while (!condition) {
        /**
         * First, process all pending events.
         */
        PendingEvent event;
        boost::unique_lock<fiberize::detail::ControlBlockMutex> lock(block->mutex);
        while (block->mailbox->dequeue(event)) {
            if (!block->handlersInitialized)
                initializeHandlers();

            lock.unlock();
            try {
                detail::dispatchEvent(event);
            } catch (...) {
                if (event.freeData)
                    event.freeData(event.data);
                throw;
            }
            if (event.freeData)
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
        scheduler()->suspend(std::move(lock));
    }
}

FiberRef self() {
    return FiberRef(std::make_shared<fiberize::detail::LocalFiberRef>(system(), detail::controlBlock()));
}

namespace detail {

fiberize::detail::ControlBlock* controlBlock() {
    return scheduler()->currentControlBlock();
}

static void collectGarbage(fiberize::detail::HandlerBlock& block) {
    size_t i = 0;

    const size_t n = block.size();
    for (size_t j = 0; j < n; ++j) {
        if (!block[j]->isDestroyed()) {
            block[i].reset(block[j].release());
            ++i;
        }
    }

    block.resize(i);
}

void dispatchEvent(const PendingEvent& event) {
    auto block = controlBlock();

    /**
     * Find a handler block.
     */
    auto handlersIt = block->handlers.find(event.path);
    if (handlersIt == block->handlers.end())
        return;
    fiberize::detail::HandlerBlock& handlers = handlersIt->second;

    /**
     * GC dead handlers.
     */
    collectGarbage(handlers);

    /**
     * There are no alive handlers, remove the handler block.
     */
    if (handlers.empty()) {
        block->handlers.erase(handlersIt);
        return;
    }

    /**
     * Execute the handlers.
     */
    auto it = handlers.rbegin();
    auto end = handlers.rend();
    while (it != end) {
        (*it)->execute(event.data);
        ++it;
    }
}

HandlerRef bind(const Path& path, std::unique_ptr<fiberize::detail::Handler> handler) {
    HandlerRef ref(handler.get());
    controlBlock()->handlers[path].emplace_back(std::move(handler));
    return ref;
}

void resume(fiberize::detail::ControlBlock* controlBlock, boost::unique_lock<fiberize::detail::ControlBlockMutex> lock) {
    /// @todo mvoe more functionality here
    scheduler()->enable(controlBlock, std::move(lock));
}

} // namespace detail

} // namespace context
} // namespace fiberize
