#ifndef FIBERIZE_DETAIL_FIBERREFIMPL_HPP
#define FIBERIZE_DETAIL_FIBERREFIMPL_HPP

#include <fiberize/locality.hpp>
#include <fiberize/path.hpp>

namespace fiberize {

struct PendingEvent;

template <typename A>
class Promise;

namespace detail {

/**
 * Interface of an fiber reference implementation.
 */
class FiberRefImpl {
public:
    virtual ~FiberRefImpl() {};

    /**
     * The locality of this fiber.
     */
    virtual Locality locality() const = 0;

    /**
     * Path of this fiber.
     */
    virtual Path path() const = 0;

    /**
     * Emits an event for an appropriatly stored value.
     */
    virtual void send(const PendingEvent& pendingEvent) = 0;
};

template <typename A>
class FutureRefImpl : public virtual FiberRefImpl {
public:
    /**
     * Returns the result of this fiber, as a promise.
     */
    virtual Promise<A>* result() = 0;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_FIBERREFIMPL_HPP
