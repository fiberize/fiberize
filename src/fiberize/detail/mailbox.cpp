#include <fiberize/detail/mailbox.hpp>

namespace fiberize {
namespace detail {

Mailbox::Mailbox() : refCount(1) {}

Mailbox::~Mailbox() {
    PendingEvent event;
    while (pendingEvents.unsynchronized_pop(event)) {
        event.buffer.free();
    }
}

void Mailbox::grab() {
    std::atomic_fetch_add_explicit(&refCount, 1lu, std::memory_order_relaxed);
}

bool Mailbox::drop() {
    if (std::atomic_fetch_sub_explicit(&refCount, 1lu, std::memory_order_release) == 1) {
         std::atomic_thread_fence(std::memory_order_acquire);
         delete this;
         return true;
    }
    return false;
}

} // namespace detail    
} // namespace fiberize
