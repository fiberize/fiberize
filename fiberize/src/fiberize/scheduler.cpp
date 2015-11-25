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

void Scheduler::enable(detail::ControlBlock* controlBlock, boost::unique_lock<detail::ControlBlockMutex>&& lock) {
    assert(controlBlock->status == detail::Suspended);
    if (controlBlock->isFiber()) {
        /**
         * If this is a fiber, forward it to a thread that is executing fibers.
         */
        enableFiber(static_cast<detail::FiberControlBlock*>(controlBlock), std::move(lock));
    } else {
        assert(controlBlock->isThread());
        auto threadControlBlock = static_cast<detail::ThreadControlBlock*>(controlBlock);

        /**
         * Switch the status to scheduled and give up the lock.
         */
        threadControlBlock->status = detail::Scheduled;
        lock.unlock();

        /**
         * Wake up the thread.
         */
        threadControlBlock->wakeUp();
    }
}

thread_local Scheduler* Scheduler::current_ = nullptr;

} // namespace fiberize
