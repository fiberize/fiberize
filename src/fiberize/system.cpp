#include <fiberize/system.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/context.hpp>

#include <thread>

namespace fiberize {
    
System::System(): System(std::thread::hardware_concurrency()) {
}

System::System(uint32_t macrothreads): stackAllocator(1024) {
    // Spawn the executors.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        std::unique_ptr<detail::Executor> executor(new detail::Executor());
        executors.push_back(std::move(executor));
    }
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

thread_local std::default_random_engine System::random;
    
} // namespace fiberize
