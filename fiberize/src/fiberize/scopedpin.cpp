#include <fiberize/scopedpin.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/detail/task.hpp>

namespace fiberize {

ScopedPin::ScopedPin() {
    if (context::scheduler()->currentTask()->pin == nullptr) {
        context::scheduler()->currentTask()->pin = context::scheduler();
        wasPinned = false;
    } else {
        wasPinned = true;
    }
}

ScopedPin::~ScopedPin() {
    if (!wasPinned) {
        context::scheduler()->currentTask()->pin = nullptr;
    }
}

} // namespace fiberize
