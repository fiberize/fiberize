#include <fiberize/fiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {

AnyFiberRef::AnyFiberRef() : impl_(new detail::DevNullFiberRef) {}
    
} // namespace fiberize
