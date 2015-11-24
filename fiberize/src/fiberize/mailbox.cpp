#include <fiberize/mailbox.hpp>

namespace fiberize {

Mailbox::~Mailbox() {
}

DequeMailbox::~DequeMailbox() {
    clear();
}

void DequeMailbox::enqueue(const PendingEvent& event) {
    pendingEvents.push_back(event);
}

bool DequeMailbox::dequeue(PendingEvent& event) {
    if (pendingEvents.empty()) {
        return false;
    } else {
        event = pendingEvents.front();
        pendingEvents.pop_front();
        return true;
    }
}

void DequeMailbox::clear() {
    for (auto& event : pendingEvents) {
        if (event.freeData != nullptr)
            event.freeData(event.data);
    }
}

} // namespace fiberize
