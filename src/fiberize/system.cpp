#include <fiberize/system.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/context.hpp>

#include <thread>

namespace fiberize {
    
thread_local std::default_random_engine random;

System::System(): System(std::thread::hardware_concurrency()) {
}

System::System(uint32_t macrothreads)
    : stackAllocator(1024 * 16)
    , mainMailbox(new LockfreeQueueMailbox())
    , mainContext_(mainMailbox)
    , shuttingDown(false) {
    // Spawn the executors.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        std::unique_ptr<detail::Executor> executor(new detail::Executor());
        executors.push_back(std::move(executor));
    }
}

FiberRef System::mainFiber() const {
    return FiberRef(std::make_shared<detail::LocalFiberRef>(mainMailbox));
}

void System::fiberRunner(intptr_t) {
    try {
        Context context(detail::Executor::current->currentControlBlock()->mailbox);
        
        // TODO: handle return values
        Buffer buffer = detail::Executor::current->currentControlBlock()->fiber->runStored();
        buffer.free();
    } catch (...) {
        // TODO: handle fiber failures.
    }
    
    detail::Executor::current->currentControlBlock()->finished = true;
    detail::Executor::current->suspend();
}
    
} // namespace fiberize
