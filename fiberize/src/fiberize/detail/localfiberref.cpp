#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/system.hpp>
#include <iostream>

namespace fiberize {
namespace detail {
        
Locality LocalFiberRef::locality() const {
    return Local;
}

Path LocalFiberRef::path() const {
    return block->path;
}

LocalFiberRef::LocalFiberRef(System* system, const std::shared_ptr<ControlBlock>& block)
    : system(system), block(block) {}

void LocalFiberRef::emit(const PendingEvent& pendingEvent) {
    boost::shared_lock<boost::upgrade_mutex> shared_lock(block->mutex);
    block->mailbox->enqueue(pendingEvent);

    if (block->status == Suspended) {
        boost::upgrade_lock<boost::upgrade_mutex> upgrade_lock(std::move(shared_lock), boost::try_to_lock);
        if (upgrade_lock.owns_lock()) {
            boost::unique_lock<boost::upgrade_mutex> unique_lock(std::move(upgrade_lock));
            system->schedule(block, std::move(unique_lock));
        }
    }
}

} // namespace detail
} // namespace fiberize
