#include <fiberize/fibersystem.hpp>
#include <fiberize/context.hpp>
#include <fiberize/detail/fiberscheduler.hpp>

#include <thread>
#include <chrono>

#include <boost/uuid/uuid_generators.hpp>

namespace fiberize {

FiberSystem::FiberSystem() : FiberSystem(std::thread::hardware_concurrency()) {}

FiberSystem::FiberSystem(uint32_t macrothreads)
    : shuttingDown_(false)
#ifdef FIBERIZE_VALGRIND
    , seedGenerator(std::chrono::system_clock::now().time_since_epoch().count())
#endif
{
    /**
     * Generate the uuid.
     */
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
    shuttingDown_ = true;
}

bool FiberSystem::shuttingDown() const {
    return shuttingDown_;
}

boost::uuids::uuid FiberSystem::uuid() const {
    return uuid_;
}
    
} // namespace fiberize
