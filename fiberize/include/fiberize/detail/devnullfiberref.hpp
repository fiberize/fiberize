#ifndef FIBERIZE_DETAIL_DEVNULLFIBERREF_HPP
#define FIBERIZE_DETAIL_DEVNULLFIBERREF_HPP

#include <fiberize/detail/fiberrefimpl.hpp>
#include <fiberize/exceptions.hpp>

namespace fiberize {

template <typename A>
class Promise;

namespace detail {

class DevNullFiberRef : public virtual FiberRefImpl {
public:
    Locality locality() const override;
    void send(const PendingEvent& pendingEvent) override;
    Path path() const override;
};

extern DevNullFiberRef devNullFiberRef;

template <typename A>
class DevNullFutureRef : public DevNullFiberRef, public FutureRefImpl<A> {
public:
    A await() override {
        throw NullAwaitable();
    }
};

template <typename A>
DevNullFutureRef<A> devNullFutureRef = {};

} // namespace detail    
} // namespace fiberize

#endif // FIBERIZE_DETAIL_DEVNULLFIBERREF_HPP

