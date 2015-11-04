#ifndef FIBERIZE_SENDABLE_HPP
#define FIBERIZE_SENDABLE_HPP

#include <cinttypes>
#include <cstring>

#include <boost/type_traits.hpp>

#include <fiberize/buffer.hpp>
#include <fiberize/types.hpp>

namespace fiberize {

enum Locality : uint8_t {
    DevNull = 0,
    Local = 1,
    Interprocess = 2,
    Remote = 3
};

template <typename A, typename = std::enable_if_t<true>>
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
     * Loads a value from the buffer.
     */
    static void restore(Buffer buffer, A* output);
    
    /**
     * Destroys a value, without deallocating.
     */
    static void destroy(A* value);
};

/**
 * Sendable implementation for PODs, for any locality.
 */
template <typename A>
struct Sendable<A, std::enable_if_t<boost::is_pod<A>::value>> {
    
    static Buffer store(const A& value) {
        return Buffer::copyFrom(&value, sizeof(A));
    }
    
    static Buffer store(A&& value) {
        return Buffer::copyFrom(&value, sizeof(A));        
    }
        
    static void load(Buffer buffer, A* output) {
        buffer.copyTo(output, sizeof(A));
    }
    
    static void destroy(A*) {
        // Noop.
    }
    
};

template <>
struct Sendable<Void> {

    static Buffer store(const Void& value) {
        abort();
    }
    
    static Buffer store(Void&& value) {
        abort();
    }
        
    static void load(Buffer buffer, Void* output) {
        abort();
    }
    
    static void destroy(Void*) {
        abort();
    }
    
};

} // namespace fiberize

#endif // FIBERIZE_SENDABLE_HPP
