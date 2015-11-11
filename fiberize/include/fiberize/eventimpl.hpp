#ifndef FIBERIZE_EVENTIMPL_HPP
#define FIBERIZE_EVENTIMPL_HPP

#include <fiberize/event.hpp>
#include <fiberize/fibercontext.hpp>

namespace fiberize {

/**
 * Waits until an event occurs and returns its value.
 */
template <typename A>
A Event<A>::await() const {
    auto handler = bind([] (const A& value) {
        FiberContext::current()->super();
        throw EventFired{value};
    });

    try {
        return FiberContext::current()->processForever().absurd<A>();
    } catch (const EventFired& eventFired) {
        return eventFired.value;
    }
}

/**
 * Binds an event to a handler.
 */
template <typename A>
template <typename... Args>
HandlerRef Event<A>::bind(Args&&... args) const {
    detail::Handler* handler = new detail::TypedHandler<A>(std::forward<Args>(args...)...);
    return FiberContext::current()->bind(path(), handler);
}

} // namespace fiberize

#endif // FIBERIZE_EVENTIMPL_HPP

