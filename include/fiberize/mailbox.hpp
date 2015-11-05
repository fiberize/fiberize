#ifndef FIBER_MAILBOX_HPP
#define FIBER_MAILBOX_HPP

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>

#include <boost/circular_buffer.hpp>
#include <boost/lockfree/queue.hpp>

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
     * Creates the mailbox with one reference.
     */
    Mailbox();

    /**
     * Destroys the mailbox and frees all buffers.
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
     * Grabs a reference.
     */
    void grab();
    
    /**
     * Drops a reference. Returns whether the object was destroyed.
     */
    bool drop();

private:
    /**
     * The number of fiber references pointing to this block.
     */
    std::atomic<std::size_t> refCount;  
};

class BlockingCircularBufferMailbox : public Mailbox {
public:
    virtual ~BlockingCircularBufferMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);
    
private:
    std::mutex mutex;
    boost::circular_buffer<PendingEvent> pendingEvents;
};

class LockfreeQueueMailbox : public Mailbox {
public:
    LockfreeQueueMailbox();
    virtual ~LockfreeQueueMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);    
    
private:
    boost::lockfree::queue<PendingEvent*> pendingEvents;
};

} // namespace fiberize

#endif // FIBER_MAILBOX_HPP
