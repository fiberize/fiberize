#include <fiberize/mailbox.hpp>

namespace fiberize {

Mailbox::~Mailbox() {
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

template class MailboxPool<BoostLockfreeQueueMailbox>;
template class MailboxPool<MoodyCamelConcurrentQueueMailbox>;

} // namespace fiberize
