#ifndef FIBERIZE_DETAIL_ENTITYTRAITS_HPP
#define FIBERIZE_DETAIL_ENTITYTRAITS_HPP

#include <fiberize/path.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/devnullfiberref.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {
namespace detail {

struct FiberTraits {
    typedef FiberRef RefType;
    typedef detail::FiberControlBlock ControlBlockType;

    inline static RefType devNullRef() {
        auto impl = std::shared_ptr<detail::FiberRefImpl>(
            std::shared_ptr<detail::FiberRefImpl>(), &devNullFiberRef);
        return FiberRef(impl);
    }

    inline static RefType localRef(FiberSystem* system, ControlBlockType* block) {
        auto impl = std::make_shared<detail::LocalFiberRef>(system, block);
        return FiberRef(impl);
    }

    inline static ControlBlockType*
    newControlBlock(const Path& path, Scheduler* bound, std::unique_ptr<Mailbox> mailbox, std::unique_ptr<Runnable> runnable) {
        auto block = new detail::FiberControlBlock;
        block->refCount = 1;
        block->bound = bound;
        block->path = path;
        block->mailbox = std::move(mailbox);
        block->runnable = std::move(runnable);
        block->status = detail::Suspended;
        block->reschedule = false;
        return block;
    }
};

template <typename A>
struct FutureTraits {
    typedef FutureRef<A> RefType;
    typedef detail::FutureControlBlock<A> ControlBlockType;

    static RefType devNullRef() {
        auto impl = std::shared_ptr<detail::FutureRefImpl<A>>(
            std::shared_ptr<detail::FutureRefImpl<A>>(), &devNullFutureRef<A>);
        return FutureRef<A>(impl);
    }

    static RefType localRef(FiberSystem* system, ControlBlockType* block) {
        auto impl = std::make_shared<detail::LocalFutureRef<A>>(system, block);
        return FutureRef<A>(impl);
    }

    static ControlBlockType*
    newControlBlock(const Path& path, Scheduler* bound, std::unique_ptr<Mailbox> mailbox, std::unique_ptr<Runnable> runnable) {
        auto block = new detail::FutureControlBlock<A>;
        block->refCount = 1;
        block->bound = bound;
        block->path = path;
        block->mailbox = std::move(mailbox);
        block->runnable = std::move(runnable);
        block->status = detail::Suspended;
        block->reschedule = false;
        return block;
    }
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_ENTITYTRAITS_HPP
