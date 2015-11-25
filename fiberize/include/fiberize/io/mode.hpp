#ifndef FIBERIZE_IO_MODE_HPP
#define FIBERIZE_IO_MODE_HPP

namespace fiberize {
namespace io {

/**
 * Executing an IO operation in await mode will block the fiber until the operation is done,
 * while processing messages and allowing other fibers to execute.
 *
 * This mode is usually the default.
 */
class Await {};
extern Await await;

/**
 * Exeuting an IO operation in block mode will block the fiber and the thread it is executing on.
 * This version does not process messages and does not allow other fibeers to execute on this core.
 *
 * This mode should be used for inexpensive and predictable IO operations, especially filesystem
 * operations. Asynchronous FS operations are currently implemented with a worker pool. The cost
 * of sending the job to a worker and thread synchronization can outweight the benefit.
 */
class Block {};
extern Block block;

/**
 * Executing an IO operation in defer mode won't block the fiber and won't process messages.
 * Instead it starts the IO operation asynchronously and reports the result with a promise.
 */
class Async {};
extern Async async;

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_MODE_HPP

