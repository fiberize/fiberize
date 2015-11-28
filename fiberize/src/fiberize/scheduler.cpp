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
    if (controlBlock->pin == nullptr) {
        /**
         * Only fibers can be unpinned.
         */
        assert(dynamic_cast<detail::FiberControlBlock*>(controlBlock) != nullptr);
        enableFiber(static_cast<detail::FiberControlBlock*>(controlBlock), std::move(lock));
    } else if (controlBlock->pin == this) {
        /**
         * If the control block is pinned it means it must be a fiber, because a
         * thread scheduler would override the enable() function and intercept
         * the thread.
         * TODO: refactor, this is not nice
         */
        assert(dynamic_cast<detail::FiberControlBlock*>(controlBlock) != nullptr);
        enableFiber(static_cast<detail::FiberControlBlock*>(controlBlock), std::move(lock));
    } else {
        /**
         * Forward the control block to its pinned scheduler.
         */
        controlBlock->pin->enable(controlBlock, std::move(lock));
    }
}

thread_local Scheduler* Scheduler::current_ = nullptr;

} // namespace fiberize
