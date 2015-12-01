/**
 * Task scheduler.
 *
 * @file scheduler.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/scheduler.hpp>
#include <thread>
#include <chrono>

using namespace std::literals;

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

void Scheduler::idle(uint64_t& idleStreak) {
    if (idleStreak <= 16) {
        // Nothing.
    } else if (idleStreak <= 64) {
        // Yield.
        std::this_thread::yield();
    } else if (idleStreak <= 1024 * 1024) {
        // Nanosleep.
        std::this_thread::sleep_for(1ns);
    } else {
        // Millisleep.
        std::this_thread::sleep_for(1ms);
    }

    idleStreak += 1;
}

void Scheduler::kill(detail::Task* task, std::unique_lock<detail::TaskMutex>&& lock) {
    task->status = detail::Dead;
    task->scheduled = false;
    task->mailbox->clear();
    lock.unlock();
    task->runnable.reset();
    task->handlers.clear();
    task->drop();
}

thread_local Scheduler* Scheduler::current_ = nullptr;

} // namespace fiberize
