#ifndef FIBERIZE_DETAIL_CONTROLBLOCK_HPP
#define FIBERIZE_DETAIL_CONTROLBLOCK_HPP

#include <boost/context/all.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/promise.hpp>
#include <fiberize/detail/fiberbase.hpp>

#include <iostream>
#include <limits>

#include <boost/thread/shared_mutex.hpp>

namespace fiberize {

class FiberContext;

namespace detail {

class FiberBase;
class Executor;
class SomePromise;

enum LifeStatus : uint8_t {
    Suspended,
    Scheduled,
    Running,
    Dead
};

typedef boost::upgrade_mutex ControlBlockMutex;

struct ControlBlock {
    /**
     * The stack of this fiber.
     */
    boost::context::stack_context stack;
    
    /**
     * The last saved context.
     */
    boost::context::fcontext_t context;
    
    /**
     * Path to this fiber.
     */
    Path path;
    
    /**
     * Mailbox attached to this control block.
     */
    std::unique_ptr<Mailbox, MailboxDeleter> mailbox;
    
    /**
     * The fiber implementation.
     */
    std::unique_ptr<FiberBase> fiber;
    
    /**
     * Promise that will contain the result of this fiber.
     */
    std::unique_ptr<SomePromise> result;

    /**
     * Status of this fiber.
     */
    LifeStatus status;

    /**
     * Lock used during status change.
     */
    ControlBlockMutex mutex;

    /**
     * Fiber context attached to this block.
     */
    FiberContext* fiberContext;

    /**
     * Whether a block should be rescheduled after a jump.
     */
    bool reschedule;

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
 
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROLBLOCK_HPP
