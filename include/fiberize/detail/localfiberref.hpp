#ifndef FIBERIZE_DETAIL_LOCALACTORREF_HPP
#define FIBERIZE_DETAIL_LOCALACTORREF_HPP

#include <fiberize/fiberref.hpp>

namespace fiberize {
namespace detail {    

class LocalFiberRef : public FiberRefImpl {
public:
    LocalFiberRef(Mailbox* mailbox);
    ~LocalFiberRef();
    
    // FiberRefImpl
    virtual Locality locality() const;
    virtual void emit(const PendingEvent& pendingEvent);

    Mailbox* mailbox();
    
private:
    Mailbox* mailbox_;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LOCALACTORREF_HPP
