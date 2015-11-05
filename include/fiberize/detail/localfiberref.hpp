#ifndef FIBERIZE_DETAIL_LOCALACTORREF_HPP
#define FIBERIZE_DETAIL_LOCALACTORREF_HPP

#include <fiberize/fiberref.hpp>

namespace fiberize {
namespace detail {    

class LocalFiberRef : public FiberRefImpl {
public:
    LocalFiberRef(const Path& path, Mailbox* mailbox);
    ~LocalFiberRef();
    
    // FiberRefImpl
    virtual Locality locality() const;
    virtual Path path() const;
    virtual void emit(const PendingEvent& pendingEvent);

    Mailbox* mailbox();
    
private:
    Path path_;
    Mailbox* mailbox_;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LOCALACTORREF_HPP
