#ifndef FIBERIZE_CONTEXT_HPP
#define FIBERIZE_CONTEXT_HPP

#include <fiberize/mailbox.hpp>

namespace fiberize {

class Context {
public:
    /**
     * Creates a new context attached to the given mailbox.
     */
    Context(Mailbox* mailbox);
    
    /**
     * Destroys the context.
     */
    ~Context();
    
    /**
     * Yields control to the event loop.
     */
    void yield();
    
    /**
     * The current context.
     */
    static thread_local Context* current;
    
private:
    Mailbox* mailbox_;
};
    
} // namespace fiberize

#endif // FIBERIZE_CONTEXT_HPP
