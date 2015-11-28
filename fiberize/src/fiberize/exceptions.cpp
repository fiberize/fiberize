#include <fiberize/exceptions.hpp>

namespace fiberize {

Killed::Killed()
    : runtime_error("Killed")
    {}

NullAwaitable::NullAwaitable()
    : runtime_error("The awaitable will never yield a value")
    {}

} // namespace fiberize
