#include <fiberize/fibersystem.hpp>
#include <fiberize/fibercontext.hpp>
#include <fiberize/detail/fiberbase.hpp>

#include <thread>
#include <chrono>

#include <boost/uuid/uuid_generators.hpp>

namespace fiberize {

FiberSystem::FiberSystem() : FiberSystem(std::thread::hardware_concurrency()) {}

FiberSystem::FiberSystem(uint32_t macrothreads) {
    shuttingDown = false;
    running = 0;
    roundRobinCounter = 0;

    /**
     * Generate the uuid.
     *
     * If valgrind support is enabled we cannot use std::random_device, because valgrind 3.11.0
     * doesn't recognize the rdrand instruction used in the implementation of random_device.
     */
#ifdef FIBERIZE_VALGRIND
    std::default_random_engine seedGenerator(std::chrono::system_clock::now().time_since_epoch().count());
#else
    std::random_device seedGenerator;
#endif
    std::uniform_int_distribution<uint64_t> seedDist;
    boost::random::mt19937 pseudorandom(seedDist(seedGenerator));
    boost::uuids::random_generator uuidGenerator(pseudorandom);
    uuid_ = uuidGenerator();

    allFibersFinished_ = newEvent<Unit>();

    // Spawn the executors.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        executors.emplace_back(new detail::Executor(this, seedDist(seedGenerator), i));
    }

    for (uint32_t i = 0; i < macrothreads; ++i) {
        executors[i]->start();
    }
}

FiberSystem::~FiberSystem() {
    for (detail::Executor* executor : executors) {
        executor->stop();
    }
    for (detail::Executor* executor : executors) {
        delete executor;
    }
}

void FiberSystem::shutdown() {
    shuttingDown = true;
}

Event<Unit> FiberSystem::allFibersFinished() {
    return allFibersFinished_;
}

void FiberSystem::schedule(detail::ControlBlock* controlBlock, boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    uint64_t i = roundRobinCounter++;
    executors[i % executors.size()]->schedule(controlBlock, std::move(lock));
}

void FiberSystem::subscribe(AnyFiberRef ref) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    subscribers.push_back(ref);
}

void FiberSystem::fiberFinished() {
    if (std::atomic_fetch_sub_explicit(&running, 1lu, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        std::lock_guard<std::mutex> lock(subscribersMutex);
        for (AnyFiberRef ref : subscribers)
            ref.send(allFibersFinished_);
    }
}

boost::uuids::uuid FiberSystem::uuid() const {
    return uuid_;
}

thread_local uint64_t FiberSystem::roundRobinCounter = 0;

thread_local UniqueIdentGenerator FiberSystem::uniqueIdentGenerator;

boost::fast_pool_allocator<detail::ControlBlock> FiberSystem::controlBlockAllocator;
    
} // namespace fiberize
