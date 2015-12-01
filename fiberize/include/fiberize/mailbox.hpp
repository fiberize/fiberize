#ifndef FIBER_MAILBOX_HPP
#define FIBER_MAILBOX_HPP

#include <atomic>
#include <vector>
#include <iostream>

#include <boost/circular_buffer.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread.hpp>

#include <fiberize/path.hpp>

namespace fiberize {

struct PendingEvent {
    Path path;
    void* data;
    void (*freeData)(void*);
};

class Mailbox {
public:
    /**
     * Destroys the mailbox and frees all data.
     */
    virtual ~Mailbox();
    
    /**
     * Tries to dequeue an event.
     */
    virtual bool dequeue(PendingEvent& event) = 0;
    
    /**
     * Enqueues an event.
     */
    virtual void enqueue(const PendingEvent& event) = 0;

    /**
     * Whether the mailbox is empty.
     */
    virtual bool empty() = 0;

    /**
     * Clears the mailbox.
     */
    virtual void clear() = 0;
};

class DequeMailbox : public Mailbox {
public:
    DequeMailbox();
    DequeMailbox(const DequeMailbox& other);
    DequeMailbox(DequeMailbox&& other);

    virtual ~DequeMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);
    virtual bool empty();
    virtual void clear();

private:
    bool initialized;
    union { std::deque<PendingEvent> pendingEvents; };
};

} // namespace fiberize

#endif // FIBER_MAILBOX_HPP
