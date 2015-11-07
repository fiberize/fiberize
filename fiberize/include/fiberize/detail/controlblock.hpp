#ifndef FIBERIZE_DETAIL_CONTROLBLOCK_HPP
#define FIBERIZE_DETAIL_CONTROLBLOCK_HPP

#include <boost/context/all.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/detail/fiberbase.hpp>

#include <iostream>
#include <limits>

#include <boost/thread/shared_mutex.hpp>

namespace fiberize {
namespace detail {

class FiberBase;
class Executor;

enum LifeStatus : uint8_t {
    Suspended,
    Scheduled,
    Running,
    Dead
};

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
    std::unique_ptr<Mailbox> mailbox;
    
    /**
     * The fiber implementation.
     */
    std::unique_ptr<FiberBase> fiber;
    
    /**
     * Path to the event emmitted when the fiber successfully finishes.
     */
    Path finishedEventPath;
    
    /**
     * Path to the event emitted when the fiber throws an uncaught exception.
     */
    Path crashedEventPath;
    
    /**
     * Fibers watching this fiber.
     * TODO: figure out concurrent acccess
     */
    std::vector<FiberRef> watchers;
    
    /**
     * The number of fiber references pointing to this block.
     */
    std::atomic<std::size_t> refCount;

    /**
     * Status of this fiber.
     */
    LifeStatus status;

    /**
     * Lock used during status change.
     */
    boost::upgrade_mutex mutex;

    /**
     * Executor executing this fiber.
     */
    Executor* executor;

    /**
     * Grabs a reference.
     */
    void grab();

    /**
     * Drops a reference. Returns whether the object was destroyed.
     */
    bool drop();
};
 
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROLBLOCK_HPP
