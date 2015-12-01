#include <fiberize/mailbox.hpp>

namespace fiberize {

Mailbox::~Mailbox() {
}

DequeMailbox::DequeMailbox() {
    initialized = false;
}

DequeMailbox::DequeMailbox(const DequeMailbox& other) {
    initialized = other.initialized;
    if (initialized) {
        new (&pendingEvents) std::deque<PendingEvent>(other.pendingEvents);
    }
}

DequeMailbox::DequeMailbox(fiberize::DequeMailbox&& other) {
    initialized = other.initialized;
    if (initialized) {
        new (&pendingEvents) std::deque<PendingEvent>(std::move(other.pendingEvents));
    }
}

DequeMailbox::~DequeMailbox() {
    clear();
}

void DequeMailbox::enqueue(const PendingEvent& event) {
    if (!initialized) {
        new (&pendingEvents) std::deque<PendingEvent>;
        initialized = true;
    }
    pendingEvents.push_back(event);
}

bool DequeMailbox::dequeue(PendingEvent& event) {
    if (!initialized || pendingEvents.empty()) {
        return false;
    } else {
        event = pendingEvents.front();
        pendingEvents.pop_front();
        return true;
    }
}

bool DequeMailbox::empty() {
    return !initialized || pendingEvents.empty();
}

void DequeMailbox::clear() {
    if (initialized) {
        for (auto& event : pendingEvents) {
            if (event.freeData)
                event.freeData(event.data);
        }
    }
}

} // namespace fiberize
