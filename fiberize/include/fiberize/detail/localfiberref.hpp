#ifndef FIBERIZE_DETAIL_LOCALFIBERREF_HPP
#define FIBERIZE_DETAIL_LOCALFIBERREF_HPP

#include <fiberize/detail/fiberrefimpl.hpp>

namespace fiberize {

class FiberSystem;

namespace detail {

class ControlBlock;

template <typename>
class FutureControlBlock;

class LocalFiberRef : public virtual FiberRefImpl {
public:
    LocalFiberRef(FiberSystem* system, ControlBlock* block);
    virtual ~LocalFiberRef();

    // FiberRefImpl
    Locality locality() const override;
    Path path() const override;
    void send(const PendingEvent& pendingEvent) override;

    FiberSystem* const system;
    ControlBlock* block;
};

template <typename A>
class LocalFutureRef : public LocalFiberRef, public FutureRefImpl<A> {
public:
    LocalFutureRef(FiberSystem* system, FutureControlBlock<A>* block)
        : LocalFiberRef(system, block) {}

    // FutureRefImpl<A>
    Promise<A>* result() override {
        return &static_cast<FutureControlBlock<A>*>(block)->result;
    }
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LOCALFIBERREF_HPP
