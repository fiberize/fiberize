#include <fiberize/mailbox.hpp>

namespace fiberize {

Mailbox::~Mailbox() {
}

BlockingCircularBufferMailbox::~BlockingCircularBufferMailbox() {
    clear();
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

void BlockingCircularBufferMailbox::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    for (PendingEvent& event : pendingEvents) {
        if (event.freeData != nullptr)
            event.freeData(event.data);
    }
}

BoostLockfreeQueueMailbox::BoostLockfreeQueueMailbox(): pendingEvents(0) {
}

BoostLockfreeQueueMailbox::~BoostLockfreeQueueMailbox() {
    clear();
}

void BoostLockfreeQueueMailbox::enqueue(const PendingEvent& event) {
    PendingEvent* ptr = new PendingEvent(event);
    pendingEvents.push(ptr);
}

bool BoostLockfreeQueueMailbox::dequeue(PendingEvent& event) {
    PendingEvent* ptr;
    if (pendingEvents.pop(ptr)) {
        event = *ptr;
        delete ptr;
        return true;
    } else {
        return false;
    }
}

void BoostLockfreeQueueMailbox::clear() {
    PendingEvent* event;
    while (pendingEvents.unsynchronized_pop(event)) {
        if (event->freeData)
            event->freeData(event->data);
        delete event;
    }
}

MoodyCamelConcurrentQueueMailbox::~MoodyCamelConcurrentQueueMailbox() {
    clear();
}

void MoodyCamelConcurrentQueueMailbox::enqueue(const PendingEvent& event) {
    pendingEvents.enqueue(event);
}

bool MoodyCamelConcurrentQueueMailbox::dequeue(PendingEvent& event) {
    return pendingEvents.try_dequeue(event);
}

void MoodyCamelConcurrentQueueMailbox::clear() {
    PendingEvent event;
    while (pendingEvents.try_dequeue(event)) {
        if (event.freeData != nullptr)
            event.freeData(event.data);
    }
}

template class MailboxPool<BlockingCircularBufferMailbox>;
template class MailboxPool<BoostLockfreeQueueMailbox>;
template class MailboxPool<MoodyCamelConcurrentQueueMailbox>;

} // namespace fiberize
