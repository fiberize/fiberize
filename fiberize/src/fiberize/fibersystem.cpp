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

boost::uuids::uuid FiberSystem::uuid() const {
    return uuid_;
}
    
} // namespace fiberize
