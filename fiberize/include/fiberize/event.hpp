#ifndef FIBERIZE_EVENT_HPP
#define FIBERIZE_EVENT_HPP

#include <string>

#include <fiberize/fibercontext.hpp>
#include <fiberize/path.hpp>

namespace fiberize {
    
template <typename A>
class Event {
public:
    /**
     * Creates a /dev/null event.
     */
    Event() {}

    /**
     * Creates an event with the given name.
     */
    Event(const std::string& name): path_(GlobalPath(NamedIdent(name))) {}
    
private:
    struct FromPath {};

    explicit Event(FromPath, const Path& path): path_(path) {}

public:
    /**
     * Creates an event with the given path.
     */
    static Event<A> fromPath(const Path& path) {
        return Event<A>(FromPath{}, path);
    }

    Event(const Event&) = default;
    Event(Event&&) = default;

    Event& operator = (const Event&) = default;
    Event& operator = (Event&&) = default;

    /**
     * Compares two events by comparing their paths.
     */
    bool operator == (const Event& other) const {
        return path_ == other.path_;
    }

    /**
     * Compares two events by comparing their paths.
     */
    bool operator != (const Event& other) const {
        return path_ != other.path_;
    }
    
    /**
     * Returns the path of this event.
     */
    Path path() const {
        return path_;
    }

    /**
     * Returns the hash of the path of the event.
     */
    uint64_t hash() const {
        boost::hash<Path> hasher;
        return hasher(path_);
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
    HandlerRef bind(const std::function<void (const A&)>& function) const {
        detail::Handler* handler = new detail::TypedHandler<A>(function);
        return FiberContext::current()->bind(path(), handler);
    }
    
    /**
     * Binds an event to a handler.
     */
    HandlerRef bind(std::function<void (const A&)>&& function) const {
        detail::Handler* handler = new detail::TypedHandler<A>(std::move(function));
        return FiberContext::current()->bind(path(), handler);
    }

private:
    Path path_;
};
    
} // namespace fiberize

#endif // FIBERIZE_EVENT_HPP
