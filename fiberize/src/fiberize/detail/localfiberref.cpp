#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/fibersystem.hpp>
#include <iostream>

namespace fiberize {
namespace detail {
        
LocalFiberRef::LocalFiberRef(FiberSystem* system, ControlBlock* block)
    : system(system), block(std::move(block)) {
    block->grab();
}

LocalFiberRef::~LocalFiberRef() {
    block->drop();
}

Locality LocalFiberRef::locality() const {
    return Local;
}

Path LocalFiberRef::path() const {
    return block->path;
}

void LocalFiberRef::send(const PendingEvent& pendingEvent) {
    boost::shared_lock<boost::upgrade_mutex> shared_lock(block->mutex);
    block->mailbox->enqueue(pendingEvent);

    if (block->status == Suspended) {
        boost::upgrade_lock<boost::upgrade_mutex> upgrade_lock(std::move(shared_lock), boost::try_to_lock);
        if (upgrade_lock.owns_lock()) {
            boost::unique_lock<boost::upgrade_mutex> unique_lock(std::move(upgrade_lock));
            Scheduler::current()->enable(block, std::move(unique_lock));
        }
    }
}

} // namespace detail
} // namespace fiberize
