#ifndef FIBERIZE_DETAIL_LOCALFIBERREF_HPP
#define FIBERIZE_DETAIL_LOCALFIBERREF_HPP

#include <fiberize/detail/fiberrefimpl.hpp>
#include <fiberize/detail/task.hpp>

namespace fiberize {

class FiberSystem;

namespace detail {

class Task;

template <typename>
class Future;

class LocalFiberRef : public FiberRefImpl {
public:
    LocalFiberRef(FiberSystem* system, Task* task);
    virtual ~LocalFiberRef();

    // FiberRefImpl
    Locality locality() const override;
    Path path() const override;
    void send(const PendingEvent& pendingEvent) override;

    FiberSystem* const system;
    Task* task;
};

template <typename A>
class LocalFutureRef : public FutureRefImpl<A> {
public:
    LocalFutureRef(FiberSystem* system, Future<A>* future)
        : system(system), future(future) {
        future->grab();
    }

    virtual ~LocalFutureRef() {
        future->drop();
    }

    // FutureRefImpl<A>

    Locality locality() const override {
        return Local;
    }

    Path path() const override {
        return future->path;
    }

    void send(const PendingEvent& pendingEvent) override {
        std::unique_lock<Spinlock> lock(future->spinlock);
        future->mailbox->enqueue(pendingEvent);
        context::detail::resume(future, std::move(lock));
    }

    Result<A> await() override {
        return future->result.await();
    }

    FiberSystem* const system;
    Future<A>* future;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LOCALFIBERREF_HPP
