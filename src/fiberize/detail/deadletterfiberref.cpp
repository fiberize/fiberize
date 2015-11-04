#include <fiberize/detail/deadletterfiberref.hpp>

namespace fiberize {
namespace detail {

Locality DeadLetterFiberRef::locality() const {
    return DevNull;
}

void DeadLetterFiberRef::emit(const PendingEvent& pendingEvent) {
    // Noop.
}
    
} // namespace detail    
} // namespace fiberize