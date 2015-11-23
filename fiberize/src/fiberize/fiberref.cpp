#include <fiberize/fiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {

FiberRef::FiberRef() : impl_(std::shared_ptr<detail::FiberRefImpl>(), &detail::devNullFiberRef) {}
    
} // namespace fiberize
