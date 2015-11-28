#include <fiberize/context.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/scheduler.hpp>

namespace fiberize {
namespace context {

FiberSystem* system() {
    return EventContext::current()->system;
}

Scheduler* scheduler() {
    return Scheduler::current();
}

void yield() {
    boost::unique_lock<detail::ControlBlockMutex> lock(EventContext::current()->controlBlock()->mutex);
    Scheduler::current()->yield(std::move(lock));
}

void process() {
    EventContext::current()->process();
}

void processForever() {
    EventContext::current()->processForever();
}

void processUntil(const bool& condition) {
    EventContext::current()->processUntil(condition);
}

FiberRef self() {
    return EventContext::current()->fiberRef();
}

} // namespace context
} // namespace fiberize
