#include <fiberize/eventenv.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/scheduler.hpp>

namespace fiberize {

FiberSystem* EventEnv::system() const {
    return EventContext::current()->system;
}

EventContext* EventEnv::context() const {
    return EventContext::current();
}

void EventEnv::yield() const {
    boost::unique_lock<detail::ControlBlockMutex> lock(context()->controlBlock()->mutex);
    Scheduler::current()->yield(std::move(lock));
}

void EventEnv::process() const {
    context()->process();
}

void EventEnv::processForever() const {
    context()->processForever();
}

void EventEnv::super() const {
    context()->super();
}

} // namespace fiberize
