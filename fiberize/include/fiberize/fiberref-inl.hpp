#ifndef FIBERIZE_FIBERREFINL_HPP
#define FIBERIZE_FIBERREFINL_HPP

#include <fiberize/fiberref.hpp>
#include <fiberize/event.hpp>

namespace fiberize {

template<typename A, typename... Args>
void FiberRef::send(const Event<A>& event, Args&&... args) {
    if (impl_->locality() != DevNull && event.path() != Path(DevNullPath{})) {
        PendingEvent pendingEvent;
        pendingEvent.path = event.path();
        pendingEvent.data = new A(std::forward<Args>(args)...);
        pendingEvent.freeData = [] (void* data) { delete reinterpret_cast<A*>(data); };
        impl_->send(pendingEvent);
    }
}

} // namespace fiberize

#endif // FIBERIZE_FIBERREFINL_HPP
