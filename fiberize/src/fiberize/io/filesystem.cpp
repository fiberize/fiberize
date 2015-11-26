#include <fiberize/io/filesystem.hpp>
#include <fiberize/io/detail/libuvwrapper.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/fibersystem.hpp>

namespace fiberize {
namespace io {

using namespace fiberize::detail;

File::File(int fd) : file(fd) {}

File::File(File&& other) {
    file = other.file;
    other.file = -1;
}

File& File::operator=(File&& other) {
    file = other.file;
    other.file = -1;
    return *this;
}

static File openResult(uv_fs_t* req) {
    return File(req->result);
}

template <typename Mode>
Result<File, Mode> File::open(const char* path, int flags, int mode) {
    return detail::LibUVWrapper<
        File,
        uv_fs_t,
        uv_fs_req_cleanup,
        decltype(&uv_fs_open),
        uv_fs_open,
        openResult
    >::execute<Mode>(path, flags, mode);
}

FIBERIZE_IO_DETAIL_INSTANTIATE_MODES(File, File::open, const char* path, int flags, int mode)

static void closeResult(uv_fs_t*) {}

template <typename Mode>
Result<void, Mode> File::close() {
    return detail::LibUVWrapper<
        void,
        uv_fs_t,
        uv_fs_req_cleanup,
        decltype(&uv_fs_close),
        uv_fs_close,
        closeResult
    >::execute<Mode>(file);
}

FIBERIZE_IO_DETAIL_INSTANTIATE_MODES(void, File::close)

static ssize_t readWriteResult(uv_fs_t* req) {
    return req->result;
}

template <typename Mode>
Result<ssize_t, Mode> File::read(const Buffer bufs[], uint nbufs, int64_t offset) {
    return detail::LibUVWrapper<
        ssize_t,
        uv_fs_t,
        uv_fs_req_cleanup,
        decltype(&uv_fs_read),
        uv_fs_read,
        readWriteResult
    >::execute<Mode>(file, detail::static_buffer_cast(bufs), nbufs, offset);
}

FIBERIZE_IO_DETAIL_INSTANTIATE_MODES(ssize_t, File::read, const Buffer bufs[], uint nbufs, int64_t offset)

template <typename Mode>
Result<ssize_t, Mode> File::write(const Buffer bufs[], uint nbufs, int64_t offset) {
    return detail::LibUVWrapper<
        ssize_t,
        uv_fs_t,
        uv_fs_req_cleanup,
        decltype(&uv_fs_write),
        uv_fs_write,
        readWriteResult
    >::execute<Mode>(file, detail::static_buffer_cast(bufs), nbufs, offset);
}

FIBERIZE_IO_DETAIL_INSTANTIATE_MODES(ssize_t, File::write, const Buffer bufs[], uint nbufs, int64_t offset)

int File::descriptor() {
    return file;
}

} // namespace io
} // namespace fiberize
