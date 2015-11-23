#ifndef FIBERIZE_DETAIL_OSTHREADSCHEDULER_HPP
#define FIBERIZE_DETAIL_OSTHREADSCHEDULER_HPP

#include <fiberize/detail/controlblock.hpp>
#include <fiberize/scheduler.hpp>

#include <boost/thread.hpp>

namespace fiberize {
namespace detail {

class ThreadScheduler : public Scheduler {
public:
    ThreadScheduler(FiberSystem* system, uint64_t seed, ThreadControlBlock* controlBlock);
    virtual ~ThreadScheduler();

    void enableFiber(FiberControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock) override;
    void suspend(boost::unique_lock<ControlBlockMutex>&& lock) override;
    void yield(boost::unique_lock<ControlBlockMutex>&& lock) override;
    [[ noreturn ]] void terminate() override;
    bool tryToStealTask(FiberControlBlock*& controlBlock) override;
    detail::ControlBlock* currentControlBlock() override;

private:
    ThreadControlBlock* controlBlock_;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_OSTHREADSCHEDULER_HPP
