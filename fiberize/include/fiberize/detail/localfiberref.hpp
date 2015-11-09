#ifndef FIBERIZE_DETAIL_LOCALACTORREF_HPP
#define FIBERIZE_DETAIL_LOCALACTORREF_HPP

#include <fiberize/fiberref.hpp>

namespace fiberize {
namespace detail {    

class LocalFiberRef : public FiberRefImpl {
public:
    LocalFiberRef(System* system, const std::shared_ptr<ControlBlock>& block);
    
    // FiberRefImpl
    virtual Locality locality() const;
    virtual Path path() const;
    virtual void send(const PendingEvent& pendingEvent);

    System* const system;
    std::shared_ptr<ControlBlock> const block;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LOCALACTORREF_HPP
