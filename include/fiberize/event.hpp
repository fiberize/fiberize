#ifndef FIBERIZE_EVENT_HPP
#define FIBERIZE_EVENT_HPP

#include <string>

#include <fiberize/context.hpp>

namespace fiberize {

template <typename A>
class Event {
public:
    /**
     * Creates an event with the given name.
     */
    Event(std::string&& name): name_(name) {}
    
    /**
     * Returns the name of this event.
     */
    const char* name() const {
        return name_.c_str();
    }
    
    /**
     * Returns the hash of this event.
     */
    const uint64_t hash() const {
        std::hash<std::string> hash_fn;
        return hash_fn(name_);
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
        try {
            Context::current()->yield();
        } catch (const EventFired& eventFired) {
            return eventFired.value;
        }
    }
    
private:
    std::string name_;
};
    
} // namespace fiberize

#endif // FIBERIZE_EVENT_HPP
