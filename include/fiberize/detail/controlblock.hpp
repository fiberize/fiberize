#ifndef FIBERIZE_DETAIL_CONTROL_BLOCK_HPP
#define FIBERIZE_DETAIL_CONTROL_BLOCK_HPP

#include <boost/context/all.hpp>

#include <fiberize/mailbox.hpp>
#include <fiberize/path.hpp>

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
     * Whether the fiber finished.
     */
    bool finished;
};
 
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROL_BLOCK_HPP
