#include <fiberize/context.hpp>
#include <fiberize/detail/executor.hpp>

namespace fiberize {

Context::Context(Mailbox* mailbox): mailbox_(mailbox) {
    Context::current = this;
    mailbox_->grab();
}

Context::~Context() {
    Context::current = nullptr;
    mailbox_->drop();
}

void Context::yield() {
    PendingEvent event;
    while (true) {
        while (mailbox_->dequeue(event)) {
            // TODO: handle event
        }
        
        // Suspend the current thread.
        Context::current = nullptr;
        detail::Executor::current->suspend();
        Context::current = this;
    }
}
 
thread_local Context* Context::current = nullptr;
 
}
