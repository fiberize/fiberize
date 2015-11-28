/**
 * Filesystem operations.
 *
 * @see @ref io_filesystem
 *
 * @file filesystem.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_FILESYSTEM_HPP
#define FIBERIZE_IO_FILESYSTEM_HPP

#include <uv.h>
#include <boost/concept_check.hpp>

#include <fiberize/promise.hpp>
#include <fiberize/io/mode.hpp>
#include <fiberize/io/buffer.hpp>

namespace fiberize {
namespace io {

/**
 * @page io_filesystem Filesystem
 *
 * Filesystem operations.
 *
 * The @ref fiberize/io/filesystem.hpp module implements filesystem operations, including file IO. The
 * Await and Async modes use a worker pool to execute requests. Block mode executes the
 * operation directly.
 *
 * @note This module wraps http://docs.libuv.org/en/v1.x/fs.html
 */

/**
 * Opens a new file.
 *
 * Equivalent to [open (2)](http://linux.die.net/man/2/open).
 */
template <typename Mode = Block>
Result<int, Mode> open(const char* path, int flags, int mode);

/**
 * Closes the file.
 *
 * Equivalent to [close (2)](http://linux.die.net/man/2/close).
 */
template <typename Mode = Block>
Result<void, Mode> close(int fd);

/**
 * Reads data from the file into multiple buffers.
 *
 * Equivalent to [read (2)](http://linux.die.net/man/2/read).
 */
template <typename Mode = Await>
Result<ssize_t, Mode> read(int fd, const Buffer bufs[], uint nbufs, int64_t offset);

/**
 * Writes data from multiple buffers into the file.
 *
 * Equivalent to [write (2)](http://linux.die.net/man/2/write).
 */
template <typename Mode = Await>
Result<ssize_t, Mode> write(int fd, const Buffer bufs[], uint nbufs, int64_t offset);

/**
 * Deletes a file.
 *
 * Equivalent to [unlink (2)](http://linux.die.net/man/2/unlink).
 */
template <typename Mode = Block>
Result<void, Mode> unlink(const char* path);

/**
 * Creates a directory.
 *
 * Equivalent to [mkdir (2)](http://linux.die.net/man/2/mkdir).
 *
 * @note Note @a mode is currently not implemented on Windows.
 */
template <typename Mode = Block>
Result<void, Mode> mkdir(const char* path, int mode);

/**
 * Create a unique temporary directory and returns it's path.
 *
 * Equivalent to [mkdtemp (2)](http://linux.die.net/man/2/mkdtemp).
 */
template <typename Mode = Block>
Result<std::string, Mode> mkdtemp(const char* tpl);

/**
 * Removes a directory.
 *
 * Equivalent to [rmdir (2)](http://linux.die.net/man/2/rmdir).
 */
template <typename Mode = Block>
Result<void, Mode> rmdir(const char* path);

/**
 * @todo scandir and stat (they are not trivial)
 */

/**
 * Renames a file.
 *
 * Equivalent to [rename (2)](http://linux.die.net/man/2/rename).
 */
template <typename Mode = Block>
Result<void, Mode> rename(const char* path, const char* new_path);

/**
 * Flushes a file.
 *
 * Equivalent to [fsync (2)](http://linux.die.net/man/2/fsync).
 */
template <typename Mode = Await>
Result<void, Mode> fsync(int fd);

/**
 * Flushes a file, without flushing the metadata.
 *
 * Equivalent to [fdatasync (2)](http://linux.die.net/man/2/fdatasync).
 */
template <typename Mode = Await>
Result<void, Mode> fdatasync(int fd);

/**
 * Truncates a file to the specified length.
 *
 * Equivalent to [ftruncate (2)](http://linux.die.net/man/2/ftruncate).
 */
template <typename Mode = Block>
Result<void, Mode> ftruncate(int fd, int64_t offset);

/**
 * Transfers data between file descriptors.
 *
 * Equivalent to [sendfile (2)](http://linux.die.net/man/2/sendfile).
 */
template <typename Mode = Await>
Result<ssize_t, Mode> sendfile(int out_fd, int in_fd, int64_t in_offset, size_t length);

/**
 * Checks if the user has permissions for a file. In case he doesn't an exception will be thrown.
 *
 * Equivalent to [access (2)](http://linux.die.net/man/2/access).
 */
template <typename Mode = Block>
Result<void, Mode> access(const char* path, int mode);

/**
 * Changes file permissions.
 *
 * Equivalent to [chmod (2)](http://linux.die.net/man/2/chmod).
 */
template <typename Mode = Block>
Result<void, Mode> chmod(const char* path, int mode);

/**
 * Changes file permissions.
 *
 * Equivalent to [fchmod (2)](http://linux.die.net/man/2/fchmod).
 */
template <typename Mode = Block>
Result<void, Mode> fchmod(int fd, int mode);

/**
 * Changes file last access and modification times.
 *
 * Equivalent to [utime (2)](http://linux.die.net/man/2/utime).
 */
template <typename Mode = Block>
Result<void, Mode> utime(const char* path, double atime, double mtime);

/**
 * Changes file last access and modification times.
 *
 * Equivalent to [futime (2)](http://linux.die.net/man/2/futime).
 */
template <typename Mode = Block>
Result<void, Mode> futime(int fd, double atime, double mtime);

/**
 * Makes a new name for a file.
 *
 * Equivalent to [link (2)](http://linux.die.net/man/2/link).
 */
template <typename Mode = Block>
Result<void, Mode> link(const char* path, const char* new_path);

/**
 * @see symlink
 */
enum SymlinkFlags : int {
    /**
     * This flag can be used with symlink() on Windows to specify whether
     * path argument points to a directory.
     */
    Dir = UV_FS_SYMLINK_DIR,

    /**
     * This flag can be used with symlink() on Windows to specify whether
     * the symlink is to be created using junction points.
     */
    Junction = UV_FS_SYMLINK_JUNCTION
};

/**
 * Makes a new name for a file.
 *
 * Equivalent to [symlink (2)](http://linux.die.net/man/2/symlink).
 *
 * @note On Windows the flags parameter can be specified to control how the symlink will be created:
 *  - @ref Dir : indicates that path points to a directory.
 *  - @ref Junction : request that the symlink is created using junction points.
 */
template <typename Mode = Block>
Result<void, Mode> symlink(const char* path, const char* new_path, int flags);

/**
 * Reads value of a symblic link.
 *
 * Equivalent to [readlink (2)](http://linux.die.net/man/2/readlink).
 */
template <typename Mode = Block>
Result<std::string, Mode> readlink(const char* path);

/**
 * Changes ownership of a file.
 *
 * Equivalent to [chown (2)](http://linux.die.net/man/2/chown).
 *
 * @warning Not implemented on Windows.
 */
template <typename Mode = Block>
Result<void, Mode> chmod(const char* path, uid_t owner, gid_t group);

/**
 * Changes ownership of a file.
 *
 * Equivalent to [fchown (2)](http://linux.die.net/man/2/fchown).
 *
 * @warning Not implemented on Windows.
 */
template <typename Mode = Block>
Result<void, Mode> fchmod(int fd, uid_t owner, gid_t group);

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_FILESYSTEM_HPP
