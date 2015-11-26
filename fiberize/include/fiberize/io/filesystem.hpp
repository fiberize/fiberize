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
 * Wraps a file descriptor and exposes basic file IO operations.
 *
 * @warning This class doesn't close the file on destruction. It's up to you.
 */
class File {
public:
    /**
     * Creates a file from a file descriptor.
     */
    explicit File(int fd);

    /**
     * Not allowed.
     */
    /// @{
    File(const File&) = delete;
    File& operator = (const File&) = delete;
    /// @}

    /**
     * Moves the file descriptor to another File object, and
     * sets the file descriptor of @a other to -1.
     */
    /// @{
    File(File&& other);
    File& operator = (File&& other);
    /// @}

    /**
     * Opens a new file.
     *
     * Equivalent to [open (2)](http://linux.die.net/man/2/open).
     */
    template <typename Mode = Block>
    static Result<File, Mode> open(const char* path, int flags, int mode);

    /**
     * Closes the file.
     *
     * Equivalent to [close (2)](http://linux.die.net/man/2/close).
     */
    template <typename Mode = Block>
    Result<void, Mode> close();

    /**
     * Reads data from the file into multiple buffers.
     *
     * Equivalent to [read (2)](http://linux.die.net/man/2/read).
     */
    template <typename Mode = Await>
    Result<ssize_t, Mode> read(const Buffer bufs[], uint nbufs, int64_t offset);

    /**
     * Write data from multiple buffers into the file.
     *
     * Equivalent to [write (2)](http://linux.die.net/man/2/write).
     */
    template <typename Mode = Await>
    Result<ssize_t, Mode> write(const Buffer bufs[], uint nbufs, int64_t offset);

    /**
     * Returns the file descriptor of this file.
     */
    int descriptor();

private:
    uv_file file;
};

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_FILESYSTEM_HPP
