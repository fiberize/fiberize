#ifndef FIBERIZE_EVENTINL_HPP
#define FIBERIZE_EVENTINL_HPP

#include <boost/optional.hpp>

#include <fiberize/event.hpp>
#include <fiberize/context.hpp>

namespace fiberize {

template <typename A>
A Event<A>::await() const {
    bool condition = false;
    boost::optional<A> result;

    HandlerRef handler = bind([&] (const A& value) {
        condition = true;
        result = value;
        handler.release();
    });

    context::processUntil(condition);
    return result.value();
}

template <>
void Event<void>::await() const;

template <typename A>
template <typename... Args>
HandlerRef Event<A>::bind(Args&&... args) const {
    std::unique_ptr<detail::Handler> handler(
        new detail::TypedHandler<A>(std::forward<Args>(args...)...));
    return context::detail::bind(path(), std::move(handler));
}

} // namespace fiberize

#endif // FIBERIZE_EVENTINL_HPP

