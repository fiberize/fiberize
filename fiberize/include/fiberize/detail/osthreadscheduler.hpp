#ifndef FIBERIZE_DETAIL_OSTHREADSCHEDULER_HPP
#define FIBERIZE_DETAIL_OSTHREADSCHEDULER_HPP

#include <fiberize/detail/controlblock.hpp>
#include <fiberize/scheduler.hpp>

#include <boost/thread.hpp>

namespace fiberize {
namespace detail {

class OSThreadScheduler : public Scheduler {
public:
    OSThreadScheduler(FiberSystem* system, uint64_t seed, ThreadControlBlock* controlBlock);
    virtual ~OSThreadScheduler();

    virtual void enableFiber(FiberControlBlock* controlBlock, boost::unique_lock<ControlBlockMutex>&& lock);
    virtual void suspend(boost::unique_lock<ControlBlockMutex>&& lock);
    virtual void yield(boost::unique_lock<ControlBlockMutex>&& lock);
    virtual void terminate();
    virtual detail::ControlBlock* currentControlBlock();

private:
    ThreadControlBlock* controlBlock_;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_OSTHREADSCHEDULER_HPP
