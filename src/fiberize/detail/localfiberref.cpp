#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {
namespace detail {
    
Locality LocalFiberRef::locality() const {
    return Local;
}

LocalFiberRef::LocalFiberRef(Mailbox* mailbox) {
    this->mailbox_ = mailbox;
    this->mailbox_->grab();
}

LocalFiberRef::~LocalFiberRef() {
    this->mailbox_->drop();
}

void LocalFiberRef::emit(const PendingEvent& pendingEvent) {
    mailbox_->enqueue(pendingEvent);
}

Mailbox* LocalFiberRef::mailbox() {
    return mailbox_;
}

} // namespace detail
} // namespace fiberize
