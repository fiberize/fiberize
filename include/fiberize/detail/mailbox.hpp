#ifndef FIBER_DETAIL_FIBERMAILBOX_HPP
#define FIBER_DETAIL_FIBERMAILBOX_HPP

#include <atomic>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <fiberize/sendable.hpp>

namespace fiberize {
namespace detail {

struct PendingEvent {
    const char* name;
    uint64_t hash;
    Buffer buffer;
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
    ~Mailbox();
        
    /**
     * Event queue.
     */
    boost::lockfree::queue<PendingEvent> pendingEvents;
    
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

} // namespace detail
} // namespace fiberize

#endif // FIBER_DETAIL_FIBERMAILBOX_HPP
