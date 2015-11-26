#ifndef FIBERIZE_IO_MODE_HPP
#define FIBERIZE_IO_MODE_HPP

#include <memory>

#include <fiberize/promise.hpp>

namespace fiberize {
namespace io {

/**
 * Executing an IO operation in await mode will block the fiber until the operation is done,
 * while processing messages and allowing other fibers to execute.
 *
 * This mode is usually the default.
 */
class Await {};

/**
 * Exeuting an IO operation in block mode will block the fiber and the thread it is executing on.
 * This version does not process messages and does not allow other fibeers to execute on this core.
 *
 * This mode should be used for inexpensive and predictable IO operations, especially filesystem
 * operations. Asynchronous FS operations are currently implemented with a worker pool. The cost
 * of sending the job to a worker and thread synchronization can outweight the benefit.
 */
class Block {};

/**
 * Executing an IO operation in defer mode won't block the fiber and won't process messages.
 * Instead it starts the IO operation asynchronously and reports the result with a promise.
 */
class Async {};

namespace detail {

template <typename Value, typename Mode>
struct ResultImpl {
};

template <typename Value>
struct ResultImpl<Value, Await> {
    typedef Value Type;
};

template <typename Value>
struct ResultImpl<Value, Block> {
    typedef Value Type;
};

template <typename Value>
struct ResultImpl<Value, Async> {
    typedef std::shared_ptr<Promise<Value>> Type;
};

} // namespace detail

/**
 * A helper used to choose the right result type based on IO mode. Await and Block modes
 * return the value, while Async returns a pointer to a promise.
 */
template <typename Value, typename Mode>
using Result = typename detail::ResultImpl<Value, Mode>::Type;

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_MODE_HPP

