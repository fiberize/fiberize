/**
 * Simple buffer.
 *
 * @file buffer.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_BUFFER_HPP
#define FIBERIZE_IO_BUFFER_HPP

#include <uv.h>

namespace fiberize {
namespace io {

/**
 * Simple pointer to a memory fragment with a known size.
 *
 * @note Buffer does not manage memory - it's just a pointer and size.
 */
class Buffer : private uv_buf_t {
public:
    /**
     * Creates a buffer from an existing memory region.
     */
    explicit Buffer(char* data, uint lenght);

    /**
     * Copies the pointer and size, without copying any data.
     */
    /// @{
    Buffer(const Buffer&) = default;
    Buffer& operator = (const Buffer&) = default;
    /// @}

    /**
     * Returns the pointer to the buffer data.
     */
    char* data() const;

    /**
     * Returns the length of this buffer.
     */
    uint length() const;
};

namespace detail {

/**
 * Casts an array of Buffer to an array of uv_buf_t.
 */
const uv_buf_t* static_buffer_cast(const Buffer* bufs);

} // namespace detail
} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_BUFFER_HPP
