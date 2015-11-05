#ifndef FIBERIZE_DETAIL_CONTROLBLOCK_HPP
#define FIBERIZE_DETAIL_CONTROLBLOCK_HPP

#include <boost/context/all.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>
#include <fiberize/fiberref.hpp>

namespace fiberize {
namespace detail {

class FiberBase;
    
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
    Mailbox* mailbox;
    
    /**
     * The fiber implementation.
     */
    FiberBase* fiber;
    
    /**
     * Path to the event emmitted when the fiber successfully finishes.
     */
    Path finishedEventPath;
    
    /**
     * Path to the event emitted when the fiber throws an uncaught exception.
     */
    Path crashedEventPath;
    
    /**
     * The fiber that spawned this fiber.
     */
    FiberRef parent;
    
    /**
     * Whether the fiber finished or crashed.
     */
    bool exited;
};
 
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROLBLOCK_HPP
