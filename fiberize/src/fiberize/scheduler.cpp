/**
 * Task scheduler.
 *
 * @file scheduler.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/scheduler.hpp>

namespace fiberize {

Scheduler::Scheduler(FiberSystem* system, uint64_t seed)
    : system_(system), random_(seed) {}

Scheduler::~Scheduler() {}

void Scheduler::makeCurrent() {
    current_ = this;
}

void Scheduler::resetCurrent() {
    current_ = nullptr;
}

thread_local Scheduler* Scheduler::current_ = nullptr;

} // namespace fiberize
