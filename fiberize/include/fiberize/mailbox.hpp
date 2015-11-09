#ifndef FIBER_MAILBOX_HPP
#define FIBER_MAILBOX_HPP

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>

#include <boost/circular_buffer.hpp>
#include <boost/lockfree/queue.hpp>

#include <fiberize/path.hpp>
#include <moodycamel/concurrentqueue.h>

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

class BoostLockfreeQueueMailbox : public Mailbox {
public:
    BoostLockfreeQueueMailbox();
    virtual ~BoostLockfreeQueueMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);    
    
private:
    boost::lockfree::queue<PendingEvent*> pendingEvents;
};

class MoodyCamelConcurrentQueueMailbox : public Mailbox {
public:
    virtual ~MoodyCamelConcurrentQueueMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);

private:
    moodycamel::ConcurrentQueue<PendingEvent> pendingEvents;
};

} // namespace fiberize

#endif // FIBER_MAILBOX_HPP
