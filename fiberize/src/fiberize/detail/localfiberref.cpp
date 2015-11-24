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
    boost::unique_lock<ControlBlockMutex> lock(block->mutex);
    block->mailbox->enqueue(pendingEvent);

    if (block->status == Suspended) {
        Scheduler::current()->enable(block, std::move(lock));
    }
}

} // namespace detail
} // namespace fiberize
