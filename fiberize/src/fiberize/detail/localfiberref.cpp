#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/task.hpp>
#include <fiberize/fibersystem.hpp>
#include <fiberize/context.hpp>

#include <iostream>

namespace fiberize {
namespace detail {
        
LocalFiberRef::LocalFiberRef(FiberSystem* system, Task* task)
    : system(system), task(task) {
    task->grab();
}

LocalFiberRef::~LocalFiberRef() {
    task->drop();
}

Locality LocalFiberRef::locality() const {
    return Local;
}

Path LocalFiberRef::path() const {
    return task->path;
}

void LocalFiberRef::send(const PendingEvent& pendingEvent) {
    std::unique_lock<Spinlock> lock(task->spinlock);
    task->mailbox->enqueue(pendingEvent);
    context::detail::resume(task, std::move(lock));
}

} // namespace detail
} // namespace fiberize
