#ifndef FIBERIZE_FIBERREF_HPP
#define FIBERIZE_FIBERREF_HPP

#include <fiberize/types.hpp>
#include <fiberize/path.hpp>
#include <fiberize/event.hpp>
#include <fiberize/mailbox.hpp>
#include <fiberize/locality.hpp>

namespace fiberize {

class AnyFiberRef;

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
     * Path of the event triggered when the fiber finishes.
     */
    virtual Path finishedEventPath() const = 0;

    /**
     * Path of the event triggered when the fiber crashes.
     */
    virtual Path crashedEventPath() const = 0;

    /**
     * Starts watching events of this fiber.
     */
    virtual void watch(const AnyFiberRef& watcher) = 0;

    /**
     * Emits an event for an appropriatly stored value.
     */
    virtual void send(const PendingEvent& pendingEvent) = 0;
    
};
    
} // namespace detail

class AnyFiberRef {
public:
    /**
     * Creates a fiber reference pointing to /dev/null.
     */
    AnyFiberRef();

    /**
     * Creates a new fiber reference with the given implementation.
     */
    inline AnyFiberRef(std::shared_ptr<detail::FiberRefImpl> impl): impl_(impl) {}

    /**
     * Copies a fiber reference.
     */
    AnyFiberRef(const AnyFiberRef& ref) = default;

    /**
     * Moves a fiber reference.
     */
    AnyFiberRef(AnyFiberRef&& ref) = default;

    /**
     * Copies a fiber reference.
     */
    AnyFiberRef& operator = (const AnyFiberRef& ref) = default;

    /**
     * Moves a fiber reference.
     */
    AnyFiberRef& operator = (AnyFiberRef&& ref) = default;

    /**
     * Returns the path to this fiber.
     */
    inline Path path() const {
        return impl_->path();
    }

    /**
     * Returns the event triggered when the fiber crashes.
     */
    inline Event<Unit> crashed() const {
        return Event<Unit>::fromPath(impl_->crashedEventPath());
    }

    /**
     * Starts watching events of this fiber.
     */
    inline void watch(const AnyFiberRef& watcher) {
        impl_->watch(watcher);
    }

    /**
     * Emits an event.
     */
    template<typename A, typename... Args>
    void send(const Event<A>& event, Args&&... args) {
        if (impl_->locality() != DevNull && event.path() != Path(DevNullPath{})) {
            PendingEvent pendingEvent;
            pendingEvent.path = event.path();
            pendingEvent.data = new A(std::forward<Args>(args)...);
            pendingEvent.freeData = [] (void* data) { delete reinterpret_cast<A*>(data); };
            impl_->send(pendingEvent);
        }
    }

    /**
     * The internal implementation.
     */
    inline std::shared_ptr<detail::FiberRefImpl> impl() {
        return impl_;
    }

protected:
    std::shared_ptr<detail::FiberRefImpl> impl_;
};

template <typename A>
class FiberRef : public AnyFiberRef {
public:
    /**
     * Creates a fiber reference pointing to /dev/null.
     */
    FiberRef() : AnyFiberRef() {};

    /**
     * Creates a new fiber reference with the given implementation.
     */
    FiberRef(std::shared_ptr<detail::FiberRefImpl> impl): AnyFiberRef(impl) {}

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
     * Returns the event triggered when the fiber finishes.
     */
    inline Event<A> finished() const {
        return Event<A>::fromPath(impl_->finishedEventPath());
    }
};

} // namespace fiberize

#endif // FIBERIZE_FIBERREF_HPP

