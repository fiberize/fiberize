#ifndef FIBERIZE_DETAIL_CONTROLBLOCK_HPP
#define FIBERIZE_DETAIL_CONTROLBLOCK_HPP

#include <fiberize/path.hpp>
#include <fiberize/mailbox.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/promise.hpp>
#include <fiberize/detail/runnable.hpp>
#include <fiberize/detail/refrencecounted.hpp>

#include <iostream>
#include <limits>
#include <unordered_map>

#include <boost/thread.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/context/all.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

namespace fiberize {

class Runnable;
class Scheduler;

namespace detail {

enum LifeStatus : uint8_t {
    Suspended,
    Scheduled,
    Running,
    Dead
};

using ControlBlockMutex = boost::detail::spinlock;
using HandlerBlock = std::vector<std::unique_ptr<detail::Handler>>;

class ControlBlock : public ReferenceCountedAtomic {
public:
    ControlBlock() {
        status = Suspended;
        mutex.v_ = 0;
        handlersInitialized = false;
    }

    virtual ~ControlBlock() {};

    /**
     * Status of this fiber.
     */
    LifeStatus status;

    /**
     * Lock used during status change.
     */
    ControlBlockMutex mutex;

    /**
     * Path to this fiber.
     */
    Path path;

    /**
     * Scheduler this task is pinned to, or nullptr.
     */
    Scheduler* pin;
    
    /**
     * Mailbox attached to this control block.
     */
    std::unique_ptr<Mailbox> mailbox;

    /**
     * Whether the standard event handlers were initialized.
     */
    bool handlersInitialized;

    /**
     * Hash map of event handlers.
     */
    std::unordered_map<Path, detail::HandlerBlock, boost::hash<Path>> handlers;
};

class RunnableControlBlock : public ControlBlock {
public:
    /**
     * Function used to execute this runnable.
     */
    std::unique_ptr<detail::ErasedRunnable> runnable;
};

class FiberControlBlock : public RunnableControlBlock {
public:
    FiberControlBlock() {
        reschedule = false;
    }

    /**
     * The stack of this fiber.
     */
    boost::context::stack_context stack;

    /**
     * The last saved context.
     */
    boost::context::fcontext_t context;

    /**
     * Whether a block should be rescheduled after a jump.
     */
    bool reschedule;
};

template <typename A>
class FutureControlBlock : public FiberControlBlock {
public:
    /**
     * Promise that will contain the result of this future.
     */
    Promise<A> result;
};

class FiberizedControlBlock : public ControlBlock {
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROLBLOCK_HPP
