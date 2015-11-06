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

LocalFiberRef::LocalFiberRef(ControlBlock* block): block(block) {
    block->grab();
}

LocalFiberRef::~LocalFiberRef() {
    block->drop();
}

void LocalFiberRef::emit(const PendingEvent& pendingEvent) {
    block->mutex.lock_shared();
    block->mailbox->enqueue(pendingEvent);

    if (block->status == Suspended) {
        if (block->mutex.try_unlock_shared_and_lock_upgrade()) {
            block->mutex.unlock_upgrade_and_lock();
            Context::current()->system->schedule(block);
            return;
        }
    }

    block->mutex.unlock_shared();
}

} // namespace detail
} // namespace fiberize
