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

    static File open(Await, const char* path, int flags, int mode);
    static File open(Block, const char* path, int flags, int mode);
    static std::shared_ptr<Promise<File>> open(Async, const char* path, int flags, int mode);

    void close(Await);
    void close(Block);
    std::shared_ptr<Promise<Unit>> close(Async);

    ssize_t read(Await, const Buffer bufs[], uint nbufs, int64_t offset);
    ssize_t read(Block, const Buffer bufs[], uint nbufs, int64_t offset);
    std::shared_ptr<Promise<ssize_t>> read(Async, const Buffer bufs[], uint nbufs, int64_t offset);

    ssize_t write(Await, const Buffer bufs[], uint nbufs, int64_t offset);
    ssize_t write(Block, const Buffer bufs[], uint nbufs, int64_t offset);
    std::shared_ptr<Promise<ssize_t>> write(Async, const Buffer bufs[], uint nbufs, int64_t offset);

private:
    uv_file file;
};

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_FILE_HPP
