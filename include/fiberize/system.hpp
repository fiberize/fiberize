#ifndef FIBERIZE_SYSTEM_HPP
#define FIBERIZE_SYSTEM_HPP

#include <boost/context/all.hpp>

#include <fiberize/fiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {

class System {
public:
    /**
     * Starts the system with a number of macrothreads based on the number of cores.
     */
    System();
    
    /**
     * Starts the system with the given number of macrothreads.
     */
    System(uint32_t macrothreads);
    
    /**
     * Starts a fiber.
     */
    template <typename FiberImpl, typename ...Args>
    FiberRef run(Args&& ...args) {
        FiberImpl* fiber = new FiberImpl(std::forward(args)...);
        
        // Create the control block.
        detail::ControlBlock* block = new detail::ControlBlock;
        block->stack = stackAllocator.allocate();
        block->context = boost::context::make_fcontext(block->stack.sp, block->stack.size, fiberTrampoline);
        block->mailbox = new detail::Mailbox();
        block->fiber = fiber;
        
        // Send it to a TODO: random executor.
        executors[0]->execute(block);
        
        // Create a reference.
        auto impl = std::make_shared<detail::LocalFiberRef>(block->mailbox);
        return FiberRef(impl);
    }
    
private:
    /**
     * Trampoline used to start a fiber.
     */
    static void fiberTrampoline(intptr_t *data);

    /**
     * Stack allocator.
     */
    boost::context::fixedsize_stack stackAllocator;
    
    /**
     * Currently running executors.
     */
    std::vector<std::unique_ptr<detail::Executor>> executors;
};
    
} // namespace fiberize

#endif // FIBERIZE_SYSTEM_HPP
