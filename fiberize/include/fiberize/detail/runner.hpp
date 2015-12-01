/**
 * Starting tasks on different schedulers.
 *
 * @file runner.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_RUNNER_HPP
#define FIBERIZE_DETAIL_RUNNER_HPP

namespace fiberize {
namespace detail {

class Task;

void runTaskAsMicrothread(Task* task);
void runTaskAsOSThread(Task* task);

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_RUNNER_HPP
