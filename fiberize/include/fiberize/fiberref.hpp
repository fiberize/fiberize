#ifndef FIBERIZE_FIBERREF_HPP
#define FIBERIZE_FIBERREF_HPP

#include <fiberize/path.hpp>
#include <fiberize/events.hpp>
#include <fiberize/mailbox.hpp>
#include <fiberize/locality.hpp>
#include <fiberize/detail/fiberrefimpl.hpp>
#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {

template <typename A>
class Event;

template <typename A>
class Promise;

class FiberRef {
public:
    /**
     * Creates a fiber reference pointing to /dev/null.
     */
    FiberRef();

    /**
     * Creates a new fiber reference with the given implementation.
     */
    inline FiberRef(std::shared_ptr<detail::FiberRefImpl> impl): impl_(std::move(impl)) {}

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
     * Kills this fiber.
     */
    void kill() const;

    /**
     * Emits an event.
     */
    template<typename A, typename... Args>
    void send(const Event<A>& event, Args&&... args) const;

    /**
     * The internal implementation.
     */
    inline std::shared_ptr<detail::FiberRefImpl> impl() const {
        return impl_;
    }

protected:
    std::shared_ptr<detail::FiberRefImpl> impl_;
};

template <typename A>
class FutureRef : public FiberRef {
public:
    /**
     * Creates a /dev/null future.
     */
    FutureRef()
        : FiberRef(std::shared_ptr<detail::FiberRefImpl>(
            std::shared_ptr<detail::FiberRefImpl>(),
            &detail::devNullFutureRef<A>))
        , futureImpl_(&detail::devNullFutureRef<A>)
        {}

    /**
     * Creates a new future reference with the given implementation.
     */
    FutureRef(std::shared_ptr<detail::FutureRefImpl<A>> impl)
        : FiberRef(impl), futureImpl_(impl.get()) {}

    /**
     * Copies a future reference.
     */
    FutureRef(const FutureRef& ref) = default;
    
    /**
     * Moves a future reference.
     */
    FutureRef(FutureRef&& ref) = default;
    
    /**
     * Copies a future reference.
     */
    FutureRef& operator = (const FutureRef& ref) = default;
    
    /**
     * Moves a future reference.
     */
    FutureRef& operator = (FutureRef&& ref) = default;

    /**
     * Awaits for the result of this future.
     */
    A await() const {
        return futureImpl_->await();
    }

private:
    detail::FutureRefImpl<A>* futureImpl_;
};

} // namespace fiberize

#endif // FIBERIZE_FIBERREF_HPP

