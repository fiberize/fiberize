#include <fiberize/scopedpin.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/detail/controlblock.hpp>

namespace fiberize {

ScopedPin::ScopedPin() {
    if (context::scheduler()->currentControlBlock()->pin == nullptr) {
        context::scheduler()->currentControlBlock()->pin = context::scheduler();
        wasPinned = false;
    } else {
        wasPinned = true;
    }
}

ScopedPin::~ScopedPin() {
    if (!wasPinned) {
        context::scheduler()->currentControlBlock()->pin = nullptr;
    }
}

} // namespace fiberize
