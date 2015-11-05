#include <fiberize/system.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/context.hpp>

#include <thread>

#include <boost/uuid/uuid_generators.hpp>

namespace fiberize {
    
thread_local std::default_random_engine random;

System::System() : System(std::thread::hardware_concurrency()) {
    std::random_device secureRandom;
    std::uniform_int_distribution<uint64_t> seedDist;
    boost::random::mt19937 pseudorandom(seedDist(secureRandom));
    boost::uuids::random_generator uuidGenerator(pseudorandom);
    uuid_ = uuidGenerator();
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
    auto mainFiberPath = PrefixedPath(uuid(), NamedIdent("main"));
    return FiberRef(std::make_shared<detail::LocalFiberRef>(mainFiberPath, mainMailbox));
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

void System::shutdown() {
    shuttingDown = true;
};

boost::uuids::uuid System::uuid() const {
    return uuid_;
}
    
} // namespace fiberize
