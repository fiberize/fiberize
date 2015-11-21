#ifndef FIBERIZE_DETAIL_DEADLETTERFIBERREF_HPP
#define FIBERIZE_DETAIL_DEADLETTERFIBERREF_HPP

#include <fiberize/fiberref.hpp>
#include <fiberize/promise.hpp>

namespace fiberize {
namespace detail {

class DevNullFiberRef : public FiberRefImpl {
public:
    virtual Locality locality() const;
    virtual void send(const PendingEvent& pendingEvent);
    virtual Path path() const;
    virtual SomePromise* result();
};

extern DevNullFiberRef devNullFiberRef;

} // namespace detail    
} // namespace fiberize

#endif // FIBERIZE_DETAIL_DEADLETTERFIBERREF_HPP

