/**
 * Starting tasks on different schedulers.
 *
 * @file schedulertraits.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_SCHEDULERTRAITS_HPP
#define FIBERIZE_DETAIL_SCHEDULERTRAITS_HPP

namespace fiberize {
namespace detail {

class Task;

struct MultiTaskSchedulerTraits {
    static void runTask(Task* task);
};

struct SingleTaskSchedulerTraits {
    static void runTask(Task* task);
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_SCHEDULERTRAITS_HPP
