#include <fiberize/fiberref.hpp>
#include <fiberize/event.hpp>
#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {

FiberRef::FiberRef() : impl_(std::shared_ptr<detail::FiberRefImpl>(), &detail::devNullFiberRef) {}

template <>
void FiberRef::send<void>(const Event<void>& event) const {
    if (impl_->locality() != DevNull && event.path() != Path(DevNullPath{})) {
        PendingEvent pendingEvent;
        pendingEvent.path = event.path();
        pendingEvent.data = nullptr;
        pendingEvent.freeData = nullptr;
        impl_->send(pendingEvent);
    }
}

void FiberRef::kill() const {
    send(fiberize::kill);
}

} // namespace fiberize
