#include <fiberize/exceptions.hpp>

namespace fiberize {

Killed::Killed()
    : runtime_error("Killed")
    {}

} // namespace fiberize
