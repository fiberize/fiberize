#include <fiberize/fibersystem.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/detail/fiberscheduler.hpp>

#include <thread>
#include <chrono>

#include <boost/uuid/uuid_generators.hpp>

namespace fiberize {

FiberSystem::FiberSystem() : FiberSystem(std::thread::hardware_concurrency()) {}

FiberSystem::FiberSystem(uint32_t macrothreads) {
    shuttingDown = false;
    running = 0;

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

    // Spawn the schedulers.
    for (uint32_t i = 0; i < macrothreads; ++i) {
        schedulers_.emplace_back(new detail::FiberScheduler(this, seedDist(seedGenerator), i));
    }

    for (uint32_t i = 0; i < macrothreads; ++i) {
        schedulers_[i]->start();
    }
}

FiberSystem::~FiberSystem() {
    for (auto scheduler : schedulers_) {
        scheduler->stop();
    }
    for (auto scheduler : schedulers_) {
        delete scheduler;
    }
}

void FiberSystem::shutdown() {
    shuttingDown = true;
}

Event<Unit> FiberSystem::allFibersFinished() {
    return allFibersFinished_;
}

void FiberSystem::subscribe(FiberRef ref) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    subscribers.push_back(ref);
}

void FiberSystem::fiberFinished() {
    if (std::atomic_fetch_sub_explicit(&running, 1lu, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        std::lock_guard<std::mutex> lock(subscribersMutex);
        for (FiberRef ref : subscribers)
            ref.send(allFibersFinished_);
    }
}

boost::uuids::uuid FiberSystem::uuid() const {
    return uuid_;
}
    
} // namespace fiberize
