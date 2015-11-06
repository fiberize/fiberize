#include <fiberize/mailbox.hpp>

namespace fiberize {

Mailbox::~Mailbox() {
}

BlockingCircularBufferMailbox::~BlockingCircularBufferMailbox() {
    std::lock_guard<std::mutex> lock(mutex);
    for (PendingEvent& event : pendingEvents) {
        if (event.freeData != nullptr)
            event.freeData(event.data);
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
    PendingEvent* event;
    while (pendingEvents.unsynchronized_pop(event)) {        
        if (event->freeData)
            event->freeData(event->data);
        delete event;
    }
}

void LockfreeQueueMailbox::enqueue(const PendingEvent& event) {
    PendingEvent* ptr = new PendingEvent(event);
    pendingEvents.push(ptr);
}

bool LockfreeQueueMailbox::dequeue(PendingEvent& event) {
    PendingEvent* ptr;
    if (pendingEvents.pop(ptr)) {
        event = *ptr;
        delete ptr;
        return true;
    } else {
        return false;
    }
}

} // namespace fiberize
