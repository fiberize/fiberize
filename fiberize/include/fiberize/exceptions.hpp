#ifndef FIBERIZE_EXCEPTIONS_HPP
#define FIBERIZE_EXCEPTIONS_HPP

#include <stdexcept>

namespace fiberize {

/**
 * Exception thrown when a fiber is killed.
 */
class Killed : std::runtime_error {
public:
    explicit Killed();

    Killed(const Killed&) = default;
    Killed(Killed&&) = default;
};

} // namespace fiberize

#endif // FIBERIZE_EXCEPTIONS_HPP
