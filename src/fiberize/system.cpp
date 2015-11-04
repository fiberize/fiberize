#include <fiberize/system.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <thread>

namespace fiberize {
    
System::System(): System(std::thread::hardware_concurrency()) {
}

System::System(uint32_t macrothreads): stackAllocator(1024 * 1024 * 32) {
    // Spawn the executors.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        std::unique_ptr<detail::Executor> executor(new detail::Executor());
        executors.push_back(std::move(executor));
    }
}

void System::fiberTrampoline(intptr_t) {
    detail::Executor::current->currentControlBlock()->fiber->entryPoint();
}
    
} // namespace fiberize
