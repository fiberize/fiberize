/**
 * Pinning down a fiber to a scheduler.
 *
 * @file scopedpin.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_SCOPEDPIN_HPP
#define FIBERIZE_SCOPEDPIN_HPP

#include <fiberize/context.hpp>

namespace fiberize {

/**
 * As long as this object exists the current fiber
 * will not be migrated between threads.
 */
class ScopedPin {
public:
    ScopedPin();
    ~ScopedPin();

private:
    bool wasPinned;
};

} // namespace fiberize

#endif // FIBERIZE_SCOPEDPIN_HPP
