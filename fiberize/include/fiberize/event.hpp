#ifndef FIBERIZE_EVENT_HPP
#define FIBERIZE_EVENT_HPP

#include <string>

#include <fiberize/path.hpp>
#include <fiberize/handler.hpp>

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
    static Event fromPath(const Path& path) {
        return Event(FromPath{}, path);
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
    A await() const;
    
    /**
     * Binds an event to a handler.
     */
    template <typename... Args>
    HandlerRef bind(Args&&... args) const;

private:
    Path path_;
};
    
} // namespace fiberize

#endif // FIBERIZE_EVENT_HPP
