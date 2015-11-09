#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {
namespace detail {

Locality DevNullFiberRef::locality() const {
    return DevNull;
}

void DevNullFiberRef::send(const PendingEvent&) {
    // Noop.
}

Path DevNullFiberRef::path() const {
    return DevNullPath();
}

Path DevNullFiberRef::finishedEventPath() const {
    return DevNullPath();
}

Path DevNullFiberRef::crashedEventPath() const {
    return DevNullPath();
}

void DevNullFiberRef::watch(const AnyFiberRef&) {}
    
} // namespace detail    
} // namespace fiberize
