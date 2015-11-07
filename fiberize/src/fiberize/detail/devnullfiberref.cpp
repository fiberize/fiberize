#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {
namespace detail {

Locality DevNullFiberRef::locality() const {
    return DevNull;
}

void DevNullFiberRef::emit(const PendingEvent& pendingEvent) {
    // Noop.
}

Path DevNullFiberRef::path() const {
    return DevNullPath();
}
    
} // namespace detail    
} // namespace fiberize