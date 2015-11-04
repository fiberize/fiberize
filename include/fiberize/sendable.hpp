#ifndef FIBERIZE_SENDABLE_HPP
#define FIBERIZE_SENDABLE_HPP

#include <cinttypes>
#include <cstring>

#include <boost/type_traits.hpp>

#include <fiberize/buffer.hpp>

namespace fiberize {

enum Locality : uint8_t {
    Local = 0,
    Interprocess = 1,
    Remote = 2
};

template <typename A, uint8_t locality, typename = std::enable_if_t<true>>
struct Sendable {
    
    /**
     * Serializes the value to a buffer.
     */
    static Buffer store(const A& value);
    
    /**
     * Serializes the value to a buffer, possibly taking advantage of the move semantics.
     */
    static Buffer store(A&& value);
    
    /**
     * Loads a value from the buffer and deallocates the buffer.
     */
    static A restore(Buffer buffer);
    
};

/**
 * Sendable implementation for PODs, for any locality.
 */
template <typename A, uint8_t locality>
struct Sendable<A, locality, std::enable_if_t<boost::is_pod<A>::value>> {
    
    static Buffer store(const A& value) {
        return Buffer::copyFrom(&value, sizeof(A));
    }
        
    static A load(Buffer buffer) {
        A value;
        buffer.copyTo(&value, sizeof(A));
        return value;
    }
    
};

} // namespace fiberize

#endif // FIBERIZE_SENDABLE_HPP
