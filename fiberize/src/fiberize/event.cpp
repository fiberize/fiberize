#include <fiberize/event.hpp>
#include <fiberize/event-inl.hpp>
#include <fiberize/context.hpp>

namespace fiberize {

template <>
void Event<void>::await() const {
    bool condition = false;

    HandlerRef handler = bind([&] () {
        condition = true;
        handler.release();
    });

    context::processUntil(condition);
}

} // namespace fiberize
