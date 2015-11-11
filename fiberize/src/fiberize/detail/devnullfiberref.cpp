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

SomePromise* DevNullFiberRef::result() {
    // TODO: this sucks!
    return nullptr;
}
    
} // namespace detail    
} // namespace fiberize
