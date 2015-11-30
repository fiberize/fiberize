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
class DevNullFutureRef : public FutureRefImpl<A> {
public:
    Locality locality() const override {
        return DevNull;
    }

    void send(const PendingEvent&) override {
        // Ignore the event.
    }

    Path path() const override {
        return DevNullPath{};
    }

    Result<A> await() override {
        return std::make_exception_ptr(NullAwaitable{});
    }
};

template <typename A>
DevNullFutureRef<A> devNullFutureRef = {};

} // namespace detail    
} // namespace fiberize

#endif // FIBERIZE_DETAIL_DEVNULLFIBERREF_HPP

