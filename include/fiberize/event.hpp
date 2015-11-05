#ifndef FIBERIZE_EVENT_HPP
#define FIBERIZE_EVENT_HPP

#include <string>

#include <fiberize/context.hpp>
#include <fiberize/path.hpp>

namespace fiberize {

template <typename A>
class Event {
public:
    /**
     * Creates an event with the given name.
     */
    Event(const std::string& name): path_(GlobalPath(NamedIdent(name))) {}
    
    /**
     * Returns the path of this event.
     */
    Path path() const {
        return path_;
    }
        
private:
    
    struct EventFired {
        A value;
    };
    
public:
    
    /**
     * Waits until an event occurs and returns its value.
     */
    A await() const {
        void (*lol)(const A&) = [] (const A& value) {
            super();
            throw EventFired{value};
        };
        auto handler = bind(lol);
        
        try {
            Context::current()->yield();
        } catch (const EventFired& eventFired) {
            return eventFired.value;
        }
    }
    
    /**
     * Binds an event to a handler.
     */
    HandlerRef bind(const std::function<void (const A&)>& function) const {
        detail::Handler* handler = new detail::TypedHandler<A>(function);
        return Context::current()->bind(path(), handler);
    }
    
    /**
     * Binds an event to a handler.
     */
    HandlerRef bind(std::function<void (const A&)>&& function) const {
        detail::Handler* handler = new detail::TypedHandler<A>(std::move(function));
        return Context::current()->bind(path(), handler);
    }

private:
    Path path_;
};
    
} // namespace fiberize

#endif // FIBERIZE_EVENT_HPP
