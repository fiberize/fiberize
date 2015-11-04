#ifndef FIBERIZE_FIBERREF_HPP
#define FIBERIZE_FIBERREF_HPP

#include <fiberize/types.hpp>
#include <fiberize/event.hpp>
#include <fiberize/detail/mailbox.hpp>

namespace fiberize {
namespace detail {
 
/**
 * Interface of an fiber reference implementation.
 */
class FiberRefImpl {
public:
    virtual ~FiberRefImpl();
    
    /**
     * The locality of this fiber.
     */
    virtual Locality locality() const = 0;
    
    /**
     * Emits an event for an appropriatly stored value.
     */
    virtual void emit(const PendingEvent& pendingEvent) = 0;
    
};
    
} // namespace detail

class FiberRef {
public:
    
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
     * Emits an event.
     */
    template <typename A>
    void emit(const Event<A>& event, A&& value) {
        detail::PendingEvent pendingEvent;
        pendingEvent.name = event.name();
        pendingEvent.hash = event.hash();
        
        switch (impl_->locality()) {
            case Local:
                pendingEvent.buffer = Sendable<A, Local>::store(value);
                break;

            case Interprocess:
                pendingEvent.buffer = Sendable<A, Interprocess>::store(value);
                break;
                
            case Remote:
                pendingEvent.buffer = Sendable<A, Remote>::store(value);
                break;
        }
        
        impl_->emit(pendingEvent);
    }
    
    /**
     * Emits an event.
     */
    template <typename A>
    void emit(const Event<A>& event, const A& value) {
        detail::PendingEvent pendingEvent;
        pendingEvent.name = event.name();
        pendingEvent.hash = event.hash();
        
        switch (impl_->locality()) {
            case Local:
                pendingEvent.buffer = Sendable<A, Local>::store(value);
                break;

            case Interprocess:
                pendingEvent.buffer = Sendable<A, Interprocess>::store(value);
                break;
                
            case Remote:
                pendingEvent.buffer = Sendable<A, Remote>::store(value);
                break;
        }
        
        impl_->emit(pendingEvent);
    }
    
    /**
     * Emits a valueless event.
     */
    void emit(const Event<Unit>& event) {
        emit(event, {});
    }
    
    /**
     * The internal implementation.
     */
    std::shared_ptr<detail::FiberRefImpl> impl() {
        return impl_;
    }
    
private:
    std::shared_ptr<detail::FiberRefImpl> impl_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBERREF_HPP

