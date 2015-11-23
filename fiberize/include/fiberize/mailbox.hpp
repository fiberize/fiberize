#ifndef FIBER_MAILBOX_HPP
#define FIBER_MAILBOX_HPP

#include <atomic>
#include <vector>
#include <iostream>

#include <boost/circular_buffer.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread.hpp>

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

    /**
     * Clears the mailbox.
     */
    virtual void clear() = 0;
};

using MailboxDeleter = std::function<void (Mailbox*)>;

class BlockingDequeMailbox : public Mailbox {
public:
    virtual ~BlockingDequeMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);
    virtual void clear();

private:
    std::deque<PendingEvent> pendingEvents;
    boost::mutex mutex;
};

class BoostLockfreeQueueMailbox : public Mailbox {
public:
    BoostLockfreeQueueMailbox();
    virtual ~BoostLockfreeQueueMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);    
    virtual void clear();
    
private:
    boost::lockfree::queue<PendingEvent*> pendingEvents;
};

class MoodyCamelConcurrentQueueMailbox : public Mailbox {
public:
    MoodyCamelConcurrentQueueMailbox();
    virtual ~MoodyCamelConcurrentQueueMailbox();
    virtual bool dequeue(PendingEvent& event);
    virtual void enqueue(const PendingEvent& event);
    virtual void clear();

private:
    moodycamel::ConcurrentQueue<PendingEvent> pendingEvents;
};

template <typename MailboxImpl>
class MailboxPool {
public:
    std::unique_ptr<Mailbox, MailboxDeleter> allocate() {
        std::unique_ptr<Mailbox> mailbox;
        if (!pool.try_dequeue(mailbox)) {
            mailbox.reset(new MailboxImpl);
        }

        return std::unique_ptr<Mailbox, MailboxDeleter>(mailbox.release(), [this] (Mailbox* ptr) {
            ptr->clear();
            pool.enqueue(std::unique_ptr<Mailbox>(ptr));
        });
    }

    static MailboxPool current;

private:
    moodycamel::ConcurrentQueue<std::unique_ptr<Mailbox>> pool;
};

template <>
class MailboxPool<BlockingDequeMailbox> {
public:
    std::unique_ptr<Mailbox, MailboxDeleter> allocate() {
        return  std::unique_ptr<Mailbox, MailboxDeleter>(new BlockingDequeMailbox(), [] (Mailbox* ptr) {
            delete ptr;
        });
    }

    static MailboxPool current;
};

template <typename MailboxImpl>
MailboxPool<MailboxImpl> MailboxPool<MailboxImpl>::current;

} // namespace fiberize

#endif // FIBER_MAILBOX_HPP
