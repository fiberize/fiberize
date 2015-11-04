#ifndef FIBERIZE_EVENT_HPP
#define FIBERIZE_EVENT_HPP

#include <string>

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
    std::string name_;
};
    
} // namespace fiberize

#endif // FIBERIZE_EVENT_HPP
