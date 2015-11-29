/**
 * Sleeping and timers.
 *
 * @see @ref sleep
 *
 * @file sleep.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_IO_SLEEP_HPP
#define FIBERIZE_IO_SLEEP_HPP

#include <chrono>
#include <thread>

#include <fiberize/io/mode.hpp>

namespace fiberize {
namespace io {

/**
 * @defgroup io_sleep Sleeping and timers.
 * @ingroup io
 *
 * Sleeping and timers.
 *
 * The @ref fiberize/io/sleep.hpp module implements the sleep function, which can be used to:
 *
 * - put the current fiber to sleep:
 * @code
 *   using namespace std::literal;
 *   sleep<Await>(2s);
 * @endcode
 *
 * - put the OS thread executing this fiber to sleep:
 * @code
 *   using namespace std::literal;
 *   sleep<Block>(2s);
 * @endcode
 *
 * - setup a timer that will fire an event:
 * @code
 *   using namespace std::literal;
 *   sleep<Async>(2s).bind([] () {
 *       std::cout << "Timer fired!" << std::endl;
 *   });
 * @endcode
 *
 * @note Implemented using http://docs.libuv.org/en/v1.x/timer.html
 */
///@{

/**
 * Suspends the current fiber/thread for at least the given duration. In Block mode it will
 * block the thread. In Async mode it will return an event that will fire after the given duration.
 *
 * @note You might have to use std::chrono::duration_cast to cast your duration to milliseconds.
 */
template <typename Mode = Await>
IOResult<void, Mode> millisleep(const std::chrono::milliseconds& duration);

/**
 * Suspends the current fiber/thread for at least the given duration. In Block mode it will
 * block the thread. In Async mode it will return an event that will fire after the given duration.
 */
template <typename Mode = Await, typename Rep, typename Period>
IOResult<void, Mode> sleep(const std::chrono::duration<Rep, Period>& duration) {
    return millisleep<Mode>(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
}

///@}

} // namespace io
} // namespace fiberize

#endif // FIBERIZE_IO_SLEEP_HPP
