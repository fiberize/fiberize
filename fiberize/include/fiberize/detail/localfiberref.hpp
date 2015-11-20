#ifndef FIBERIZE_DETAIL_LOCALACTORREF_HPP
#define FIBERIZE_DETAIL_LOCALACTORREF_HPP

#include <fiberize/fiberref.hpp>
#include <fiberize/detail/controlblockptr.hpp>

namespace fiberize {

class FiberSystem;

namespace detail {

class LocalFiberRef : public FiberRefImpl {
public:
    LocalFiberRef(FiberSystem* system, ControlBlockPtr block);
    
    // FiberRefImpl
    virtual Locality locality() const;
    virtual Path path() const;
    virtual void send(const PendingEvent& pendingEvent);
    virtual SomePromise* result();

    FiberSystem* const system;
    ControlBlockPtr block;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LOCALACTORREF_HPP
