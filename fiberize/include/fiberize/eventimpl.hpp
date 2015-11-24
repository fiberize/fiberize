#ifndef FIBERIZE_EVENTIMPL_HPP
#define FIBERIZE_EVENTIMPL_HPP

#include <boost/optional.hpp>

#include <fiberize/event.hpp>
#include <fiberize/eventcontext.hpp>

namespace fiberize {

/**
 * Waits until an event occurs and returns its value.
 */
template <typename A>
A Event<A>::await() const {
    bool condition = false;
    boost::optional<A> result;

    HandlerRef handler = bind([&] (const A& value) {
        condition = true;
        result = value;
        handler.release();
    });

    EventContext::current()->processUntil(condition);
    return result.value();
}

/**
 * Binds an event to a handler.
 */
template <typename A>
template <typename... Args>
HandlerRef Event<A>::bind(Args&&... args) const {
    std::unique_ptr<detail::Handler> handler(
        new detail::TypedHandler<A>(std::forward<Args>(args...)...));
    return EventContext::current()->bind(path(), std::move(handler));
}

} // namespace fiberize

#endif // FIBERIZE_EVENTIMPL_HPP

