#include <fiberize/mailbox.hpp>

namespace fiberize {

Mailbox::Mailbox() : refCount(1) {}

Mailbox::~Mailbox() {
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

BlockingCircularBufferMailbox::~BlockingCircularBufferMailbox() {
    std::lock_guard<std::mutex> lock(mutex);
    for (PendingEvent& event : pendingEvents) {
        event.buffer.free();
    }
}

void BlockingCircularBufferMailbox::enqueue(const PendingEvent& event) {
    std::lock_guard<std::mutex> lock(mutex);
    pendingEvents.push_back(event);
}

bool BlockingCircularBufferMailbox::dequeue(PendingEvent& event) {
    std::lock_guard<std::mutex> lock(mutex);
    if (pendingEvents.empty()) {
        return false;
    } else {
        event = pendingEvents.front();
        pendingEvents.pop_front();
        return true;
    }
}

LockfreeQueueMailbox::LockfreeQueueMailbox(): pendingEvents(0) {
}

LockfreeQueueMailbox::~LockfreeQueueMailbox() {
    PendingEvent event;
    while (pendingEvents.unsynchronized_pop(event)) {
        event.buffer.free();
    }
}

void LockfreeQueueMailbox::enqueue(const PendingEvent& event) {
    pendingEvents.push(event);
}

bool LockfreeQueueMailbox::dequeue(PendingEvent& event) {
    return pendingEvents.pop(event);
}

} // namespace fiberize
