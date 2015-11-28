#include <fiberize/detail/threadscheduler.hpp>
#include <fiberize/detail/fiberscheduler.hpp>
#include <fiberize/fibersystem.hpp>

#include <thread>

namespace fiberize {
namespace detail {

ThreadScheduler::ThreadScheduler(FiberSystem* system, uint64_t seed, FiberizedControlBlock* controlBlock)
    : Scheduler(system, seed), controlBlock_(controlBlock)
    {}

ThreadScheduler::~ThreadScheduler() {
    if (controlBlock_ != nullptr)
        controlBlock_->drop();
}

void ThreadScheduler::enable(ControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock)
{
    assert(controlBlock->status == Suspended);
    if (controlBlock != controlBlock_) {
        Scheduler::enable(controlBlock, std::move(lock));
    } else {
        controlBlock_->status = Scheduled;
        lock.unlock();
        ioContext().stopLoop();
    }
}

void ThreadScheduler::enableFiber(FiberControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock) {
    assert(controlBlock->status == Suspended);
    const auto& schedulers = system()->schedulers();
    std::uniform_int_distribution<std::size_t> dist(0, schedulers.size() - 1);
    std::size_t i = dist(random());
    schedulers[i]->enableFiber(controlBlock, std::move(lock));
}

void ThreadScheduler::suspend(boost::unique_lock<ControlBlockMutex>&& lock) {
    controlBlock_->status = Suspended;
    lock.unlock();

    /**
     * Loop until someone breaks it.
     */
    ioContext().runLoop();

    /**
     * Change the status to running without locking.
     */
    assert(controlBlock_->status == Scheduled);
    controlBlock_->status = Running;
}

void ThreadScheduler::yield(boost::unique_lock<ControlBlockMutex>&& lock) {
    lock.unlock();
    ioContext().runLoopNoWait();
    std::this_thread::yield();
}

void ThreadScheduler::terminate() {
    controlBlock_->status = Dead;
    controlBlock_->drop();
    controlBlock_ = nullptr;
    int retval = 0;

    // NOTE: AFAIK there is no equivalent to this function in boost::thread or std.
    //       This is not an issue for now, because we only target linux.
    pthread_exit(&retval);
}

bool ThreadScheduler::tryToStealTask(FiberControlBlock*&) {
    return false;
}

ControlBlock* ThreadScheduler::currentControlBlock() {
    return controlBlock_;
}

} // namespace detail
} // namespace fiberize
