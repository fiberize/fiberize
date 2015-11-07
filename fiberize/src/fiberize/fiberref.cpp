#include <fiberize/fiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {

FiberRef::FiberRef() : impl_(new detail::DevNullFiberRef) {}
    
} // namespace fiberize
