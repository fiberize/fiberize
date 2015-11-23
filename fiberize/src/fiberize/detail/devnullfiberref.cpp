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

DevNullFiberRef devNullFiberRef;

} // namespace detail    
} // namespace fiberize
