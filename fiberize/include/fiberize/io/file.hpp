#ifndef FIBERIZE_IO_FILE_HPP
#define FIBERIZE_IO_FILE_HPP

#include <uv.h>

#include <fiberize/promise.hpp>
#include <fiberize/io/mode.hpp>
#include <fiberize/io/buffer.hpp>

namespace fiberize {
namespace io {

class File {
public:
    /**
     * Creates a file from a file descriptor.
     */
    explicit File(int fd);

    File(const File&) = delete;
    File(File&& other);

    File& operator = (const File&) = delete;
    File& operator = (File&& other);

    template <typename Mode = Block>
    static Result<File, Mode> open(const char* path, int flags, int mode);

    template <typename Mode = Block>
    Result<void, Mode> close();

    template <typename Mode = Await>
    Result<ssize_t, Mode> read(const Buffer bufs[], uint nbufs, int64_t offset);

    template <typename Mode = Await>
    Result<ssize_t, Mode> write(const Buffer bufs[], uint nbufs, int64_t offset);

private:
    uv_file file;
};

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_FILE_HPP
