#include <fiberize/io/buffer.hpp>

namespace fiberize {
namespace io {

/**
 * We are relying on somewhat uncertain assumption that Buffer and uv_buf_t
 * have the same memory representation, and so an array of Buffer elements is an
 * array of uv_buf_t elements.
 */
static_assert(
    sizeof(uv_buf_t) == sizeof(Buffer),
    "FIBERIZE BUG: sizeof(Buffer) != sizeof(uv_buf_t)\n" \
    "Please report this attaching your compiler version and system architecture."
);

Buffer::Buffer(char* data, uint length) {
    *reinterpret_cast<uv_buf_t*>(this) = uv_buf_init(data, length);
}

char* Buffer::data() const {
    return base;
}

uint Buffer::length() const {
    return len;
}

} // namespace io
} // namespace fiberize
