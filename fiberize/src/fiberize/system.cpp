#include <fiberize/system.hpp>
#include <fiberize/detail/fiberbase.hpp>
#include <fiberize/context.hpp>

#include <thread>

#include <boost/uuid/uuid_generators.hpp>

namespace fiberize {

System::System() : System(std::thread::hardware_concurrency()) {}

System::System(uint32_t macrothreads)
    : mainControlBlock(createUnmanagedBlock())
    , mainContext_(mainControlBlock, this)
    , shuttingDown(false)
    , allFibersFinished_(newEvent<Unit>())
    , running(0)
    , roundRobinCounter(0) {
    mainControlBlock->grab();

    /**
     * Generate the uuid.
     */
    std::random_device secureRandom;
    std::uniform_int_distribution<uint64_t> seedDist;
    boost::random::mt19937 pseudorandom(seedDist(secureRandom));
    boost::uuids::random_generator uuidGenerator(pseudorandom);
    uuid_ = uuidGenerator();

    // Spawn the executors.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        executors.emplace_back(new detail::Executor(this, seedDist(secureRandom), i));
    }

    for (uint32_t i = 0; i < macrothreads; ++i) {
        executors[i]->start();
    }
}

System::~System() {
    for (detail::Executor* executor : executors) {
        executor->stop();
    }
    for (detail::Executor* executor : executors) {
        delete executor;
    }
    mainControlBlock->drop();
}

FiberRef System::mainFiber() {
    return FiberRef(std::make_shared<detail::LocalFiberRef>(this, mainControlBlock));
}

Context* System::mainContext() {
    return &mainContext_;
}

void System::shutdown() {
    shuttingDown = true;
}

Event<Unit> System::allFibersFinished() {
    return allFibersFinished_;
}

void System::schedule(detail::ControlBlock* controlBlock) {
    // TODO: optimize memory order
    uint64_t i = std::atomic_fetch_add(&roundRobinCounter, 1lu);
    executors[i % executors.size()]->schedule(controlBlock);
}

void System::fiberFinished() {
    if (std::atomic_fetch_sub_explicit(&running, 1lu, std::memory_order_release) == 1) {
         std::atomic_thread_fence(std::memory_order_acquire);
         // TODO: subscription
         mainFiber().emit(allFibersFinished_);
    }
}

boost::uuids::uuid System::uuid() const {
    return uuid_;
}
    
} // namespace fiberize
