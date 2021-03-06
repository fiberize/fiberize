/**
 * Task scheduler.
 *
 * @file scheduler.cpp
 * @copyright 2015 Paweł Nowak
 */
#include <fiberize/scheduler.hpp>
#include <thread>
#include <chrono>

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
    using namespace std::literals;

    if (idleStreak <= 16) {
        // Nothing.
    } else if (idleStreak <= 64) {
        // Yield.
        std::this_thread::yield();
    } else if (idleStreak <= 64 * 1024) {
        // Nanosleep.
        std::this_thread::sleep_for(1ns);
    } else {
        // Millisleep.
        std::this_thread::sleep_for(1ms);
    }

    idleStreak += 1;
}

void Scheduler::kill(detail::Task* task, std::unique_lock<Spinlock>&& lock) {
    if (task->refCount == 0) {
        lock.release();
        delete task;
    } else {
        task->status = detail::Dead;
        task->scheduled = false;
        task->refCount += 1;
        lock.unlock();
        task->runnable.reset();
        task->mailbox->clear();
        task->handlers.clear();
        lock.lock();
        task->refCount -= 1;
        if (task->refCount == 0) {
            lock.release();
            delete task;
        }
    }
}

thread_local Scheduler* Scheduler::current_ = nullptr;

} // namespace fiberize
