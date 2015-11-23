#include <fiberize/fiber.hpp>
#include <fiberize/eventcontext.hpp>

namespace fiberize {

void Fiber::operator () () {
    try {
        run();
    } catch (...) {
        // Nothing
    }
}

FiberRef Fiber::self() const {
    return context()->fiberRef();
}

} // namespace fiberize
