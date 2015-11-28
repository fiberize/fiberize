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

/**
 * Thrown when you await on a /dev/null future.
 */
class NullAwaitable : std::runtime_error {
public:
    explicit NullAwaitable();

    NullAwaitable(const NullAwaitable&) = default;
    NullAwaitable(NullAwaitable&&) = default;
};

} // namespace fiberize

#endif // FIBERIZE_EXCEPTIONS_HPP
