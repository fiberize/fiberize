#ifndef FIBERIZE_EVENTS_HPP
#define FIBERIZE_EVENTS_HPP

#include <fiberize/event.hpp>

namespace fiberize {

/**
 * Kills the receiver by throwing the Killed exception.
 */
extern Event<void> kill;

} // namespace fiberize

#endif // FIBERIZE_EVENTS_HPP
