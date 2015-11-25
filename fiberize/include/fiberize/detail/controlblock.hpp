#ifndef FIBERIZE_DETAIL_CONTROLBLOCK_HPP
#define FIBERIZE_DETAIL_CONTROLBLOCK_HPP

#include <fiberize/path.hpp>
#include <fiberize/mailbox.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/promise.hpp>

#include <iostream>
#include <limits>

#include <boost/thread.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/context/all.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

namespace fiberize {

class Runnable;
class EventContext;
class Scheduler;

namespace detail {

enum LifeStatus : uint8_t {
    Suspended,
    Scheduled,
    Running,
    Dead
};

typedef boost::detail::spinlock ControlBlockMutex;

class ControlBlock {
public:
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
     * Scheduler this task is bound to, or nullptr.
     */
    Scheduler* bound;
    
    /**
     * Mailbox attached to this control block.
     */
    std::unique_ptr<Mailbox> mailbox;
    
    /**
     * Event context attached to this block.
     */
    EventContext* eventContext;

    // Reference counting.

    /**
     * Reference counter.
     */
    std::atomic<uint32_t> refCount;

    /**
     * Grabs a reference.
     */
    inline void grab() {
        std::atomic_fetch_add(&refCount, 1u);
    }

    /**
     * Drops a reference.
     */
    inline void drop() {
        if (std::atomic_fetch_sub_explicit(&refCount, 1u, std::memory_order_release) == 1u) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }
};

class FiberControlBlock : public ControlBlock {
public:
    /**
     * The stack of this fiber.
     */
    boost::context::stack_context stack;

    /**
     * The last saved context.
     */
    boost::context::fcontext_t context;

    /**
     * The fiber implementation.
     */
    std::unique_ptr<Runnable> runnable;

    /**
     * Whether a block should be rescheduled after a jump.
     */
    bool reschedule;
};

template <typename A>
class FutureControlBlock : public FiberControlBlock {
public:
    /**
     * Promise that will contain the result of this fiber.
     */
    Promise<A> result;
};

class ThreadControlBlock : public ControlBlock {
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROLBLOCK_HPP
