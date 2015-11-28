#include <fiberize/io/filesystem.hpp>
#include <fiberize/io/detail/libuvwrapper.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/fibersystem.hpp>

namespace fiberize {
namespace io {

#define FIBERIZE_IO_DETAIL_FS_WRAPPER(Name, Extractor, Result, Args) \
    FIBERIZE_IO_DETAIL_WRAPPER(fs, Name, Extractor, Result, Args)

static int openResult(uv_fs_t* req) {
    return req->result;
}

FIBERIZE_IO_DETAIL_FS_WRAPPER(open, openResult, int, ((const char*) path)((int) flags)((int) mode))

static void noResult(uv_fs_t*) {}

FIBERIZE_IO_DETAIL_FS_WRAPPER(close, noResult, void, ((int) fd))

static ssize_t sizeResult(uv_fs_t* req) {
    return req->result;
}

FIBERIZE_IO_DETAIL_FS_WRAPPER(read, sizeResult, ssize_t, ((int) fd)((const Buffer*) bufs)((uint) nbufs)((int64_t) offset))
FIBERIZE_IO_DETAIL_FS_WRAPPER(write, sizeResult, ssize_t, ((int) fd)((const Buffer*) bufs)((uint) nbufs)((int64_t) offset))
FIBERIZE_IO_DETAIL_FS_WRAPPER(sendfile, sizeResult, ssize_t, ((int) out_fd)((int) in_fd)((int64_t) in_offset)((size_t) length))

FIBERIZE_IO_DETAIL_FS_WRAPPER(unlink, noResult, void, ((const char*) path))
FIBERIZE_IO_DETAIL_FS_WRAPPER(mkdir, noResult, void, ((const char*) path)((int) mode))
FIBERIZE_IO_DETAIL_FS_WRAPPER(rmdir, noResult, void, ((const char*) path))
FIBERIZE_IO_DETAIL_FS_WRAPPER(rename, noResult, void, ((const char*) path)((const char*) new_path))
FIBERIZE_IO_DETAIL_FS_WRAPPER(fsync, noResult, void, ((int) fd))
FIBERIZE_IO_DETAIL_FS_WRAPPER(fdatasync, noResult, void, ((int) fd))
FIBERIZE_IO_DETAIL_FS_WRAPPER(ftruncate, noResult, void, ((int) fd)((int64_t) offset))
FIBERIZE_IO_DETAIL_FS_WRAPPER(access, noResult, void, ((const char*) path)((int) mode))
FIBERIZE_IO_DETAIL_FS_WRAPPER(chmod, noResult, void, ((const char*) path)((int) mode))
FIBERIZE_IO_DETAIL_FS_WRAPPER(fchmod, noResult, void, ((int) fd)((int) mode))
FIBERIZE_IO_DETAIL_FS_WRAPPER(utime, noResult, void, ((const char*) path)((double) atime)((double) mtime))
FIBERIZE_IO_DETAIL_FS_WRAPPER(futime, noResult, void, ((int) fd)((double) atime)((double) mtime))
FIBERIZE_IO_DETAIL_FS_WRAPPER(link, noResult, void, ((const char*) path)((const char*) new_path))
FIBERIZE_IO_DETAIL_FS_WRAPPER(symlink, noResult, void, ((const char*) path)((const char*) new_path)((int) flags))
FIBERIZE_IO_DETAIL_FS_WRAPPER(chown, noResult, void, ((const char*) path)((uid_t) owner)((gid_t) group))
FIBERIZE_IO_DETAIL_FS_WRAPPER(fchown, noResult, void, ((int) fd)((uid_t) owner)((gid_t) group))

static std::string mkdtempResult(uv_fs_t* req) {
    // TODO: this is suboptimal, we could move the path
    return req->path;
}

FIBERIZE_IO_DETAIL_FS_WRAPPER(mkdtemp, mkdtempResult, std::string, ((const char*) tpl))

static std::string readlinkResult(uv_fs_t* req) {
    return reinterpret_cast<const char*>(req->ptr);
}

FIBERIZE_IO_DETAIL_FS_WRAPPER(readlink, readlinkResult, std::string, ((const char*) path))

} // namespace io
} // namespace fiberize
