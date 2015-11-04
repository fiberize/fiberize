#ifndef FIBERIZE_BUFFER_HPP
#define FIBERIZE_BUFFER_HPP

#include <cstring>
#include <algorithm>

namespace fiberize {

/**
 * Unsafe and efficient buffer.
 */
struct Buffer {
    uint8_t isLarge;
    
    union {
        struct {
            uint64_t size;
            void* data;
        } large;
        
        struct {
            uint8_t data[sizeof(uint64_t) + sizeof(uint8_t*)];
        } small;
    };
    
    /**
     * An empty buffer.
     */
    static Buffer empty() {
        Buffer buffer;
        memset(&buffer, 0, sizeof(Buffer));
        buffer.isLarge = 0;
        return buffer;
    }
    
    /**
     * Creates a buffer from raw data.
     */
    static Buffer copyFrom(const void* data, uint64_t size) {
        Buffer buffer = empty();
        if (size < sizeof(Buffer)) {
            buffer.isLarge = 0;
            memcpy(buffer.small.data, data, size);
        } else {        
            buffer.isLarge = 1;
            buffer.large.size = size;
            buffer.large.data = malloc(size);
            memcpy(buffer.large.data, data, size);
        }
        return buffer;
    }
    
    /**
     * Copies up to size of the buffers data to the given memory region.
     */
    void copyTo(void* memory, uint64_t size) const {
        if (isLarge) {
            memcpy(memory, large.data, std::min(large.size, size));
        } else {
            memcpy(memory, small.data, size);
        }
    }

    /**
     * Frees the large buffer.
     */
    void free() {
        if (isLarge) {
            ::free(large.data);
        }
    }
};
    
}

#endif // FIBERIZE_BUFFER_HPP
