#include <fiberize/event.hpp>
#include <fiberize/event-inl.hpp>
#include <fiberize/eventcontext.hpp>

namespace fiberize {

template <>
void Event<void>::await() const {
    bool condition = false;

    HandlerRef handler = bind([&] () {
        condition = true;
        handler.release();
    });

    EventContext::current()->processUntil(condition);
}

} // namespace fiberize
