/**
 * Low level IO system.
 *
 * @see @ref io
 *
 * @file io.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_IO_HPP
#define FIBERIZE_IO_IO_HPP

/**
 * @defgroup io Low level IO system.
 * @ingroup fiberize
 *
 * Provides portable blocking, nonblocking and asynchronous IO.
 *
 * This is the low level IO system based on the [libuv](http://libuv.org/) library. It implements various interfaces,
 * including filesystem, sockets, terminals, timers, signals and processes. The library is quite low level and can
 * be cumbersome to use, as it is intended to be a backend for higher level IO libraries.
 *
 * The implementation is event driven, using mechanisms such as [epoll](http://linux.die.net/man/4/epoll). The filesystem
 * module is an exception, as linux doesn't provide reliable asynchronous file operations, so asynchronous execution
 * is achieved using a thread pool.
 *
 * There are different @ref io_modes that can be used to control how operations are executed.
 *
 * All IO functions can throw the std::system_error exception.
 */

#include <fiberize/io/mode.hpp>
#include <fiberize/io/filesystem.hpp>
#include <fiberize/io/sleep.hpp>

#endif // FIBERIZE_IO_IO_HPP
