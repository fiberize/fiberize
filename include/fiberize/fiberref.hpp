#ifndef FIBERIZE_FIBERREF_HPP
#define FIBERIZE_FIBERREF_HPP

#include <fiberize/types.hpp>
#include <fiberize/path.hpp>
#include <fiberize/event.hpp>
#include <fiberize/mailbox.hpp>

namespace fiberize {
namespace detail {
 
/**
 * Interface of an fiber reference implementation.
 */
class FiberRefImpl {
public:
    virtual ~FiberRefImpl() {};
    
    /**
     * The locality of this fiber.
     */
    virtual Locality locality() const = 0;
    
    /**
     * Path of this fiber.
     */
    virtual Path path() const = 0;
    
    /**
     * Emits an event for an appropriatly stored value.
     */
    virtual void emit(const PendingEvent& pendingEvent) = 0;
    
};
    
} // namespace detail

class FiberRef {
public:
    /**
     * Creates a fiber reference pointing to /dev/null.
     */
    FiberRef();
    
    /**
     * Creates a new fiber reference with the given implementation.
     */
    FiberRef(std::shared_ptr<detail::FiberRefImpl> impl): impl_(impl) {}
    
    /**
     * Copies a fiber reference.
     */
    FiberRef(const FiberRef& ref) = default;
    
    /**
     * Moves a fiber reference.
     */
    FiberRef(FiberRef&& ref) = default;
    
    /**
     * Copies a fiber reference.
     */
    FiberRef& operator = (const FiberRef& ref) = default;
    
    /**
     * Moves a fiber reference.
     */
    FiberRef& operator = (FiberRef&& ref) = default;
    
    /**
     * Returns the path to this fiber.
     */
    inline Path path() const {
        return impl_->path();
    }
    
    /**
     * Emits an event.
     */
    template <typename A>
    void emit(const Event<A>& event, A&& value) {
        if (impl_->locality() != DevNull && event.path() != Path(DevNullPath{})) {
            PendingEvent pendingEvent;
            pendingEvent.path = event.path();
            pendingEvent.data = new A(std::move(value));
            pendingEvent.freeData = [] (void* data) { delete reinterpret_cast<A*>(data); };
            impl_->emit(pendingEvent);
        }
    }
    
    /**
     * Emits an event.
     */
    template <typename A>
    void emit(const Event<A>& event, const A& value) {
        if (impl_->locality() != DevNull && event.path() != Path(DevNullPath{})) {
            PendingEvent pendingEvent;
            pendingEvent.path = event.path();
            pendingEvent.data = new A(value);
            pendingEvent.freeData = [] (void* data) { delete reinterpret_cast<A*>(data); };
            impl_->emit(pendingEvent);
        }
    }
    
    /**
     * Emits a valueless event.
     */
    inline void emit(const Event<Unit>& event) {
        emit(event, {});
    }
    
    /**
     * The internal implementation.
     */
    inline std::shared_ptr<detail::FiberRefImpl> impl() {
        return impl_;
    }
    
private:
    std::shared_ptr<detail::FiberRefImpl> impl_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBERREF_HPP

