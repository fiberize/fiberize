#ifndef FIBERIZE_DETAIL_DEADLETTERFIBERREF_HPP
#define FIBERIZE_DETAIL_DEADLETTERFIBERREF_HPP

#include <fiberize/fiberref.hpp>

namespace fiberize {
namespace detail {

class DevNullFiberRef : public FiberRefImpl {
    virtual Locality locality() const;
    virtual void emit(const PendingEvent& pendingEvent);
    virtual Path path() const;
};

} // namespace detail    
} // namespace fiberize

#endif // FIBERIZE_DETAIL_DEADLETTERFIBERREF_HPP

