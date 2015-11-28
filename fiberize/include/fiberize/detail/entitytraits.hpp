#ifndef FIBERIZE_DETAIL_ENTITYTRAITS_HPP
#define FIBERIZE_DETAIL_ENTITYTRAITS_HPP

#include <utility>

#include <fiberize/path.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/devnullfiberref.hpp>
#include <fiberize/detail/localfiberref.hpp>

namespace fiberize {
namespace detail {

struct FiberTraits {
    template <typename Entity>
    struct For {
        template <typename... Args>
        struct WithArgs {
            using RefType = FiberRef;
            using ControlBlockType = detail::FiberControlBlock;

            inline static RefType devNullRef() {
                auto impl = std::shared_ptr<detail::FiberRefImpl>(
                    std::shared_ptr<detail::FiberRefImpl>(), &devNullFiberRef);
                return FiberRef(impl);
            }

            inline static RefType localRef(FiberSystem* system, ControlBlockType* block) {
                auto impl = std::make_shared<detail::LocalFiberRef>(system, block);
                return FiberRef(impl);
            }

            template <typename Runnable>
            inline static ControlBlockType*
            newControlBlock(const Path& path, std::unique_ptr<Mailbox> mailbox, Scheduler* bond, Runnable runnable) {
                auto block = new detail::FiberControlBlock;
                block->bound = bond;
                block->path = path;
                block->mailbox = std::move(mailbox);
                block->runnable = makeRunnable([runnable = std::move(runnable)] () mutable {
                    try {
                        runnable();
                    } catch (...) {
                        // Nothing.
                    }
                });
                return block;
            }
        };
    };
};


template <typename A>
struct CompletePromise {
    template <typename Closure>
    static void with(Promise<A>& promise, Closure&& closure) {
        promise.tryToComplete(closure());
    }
};

template <>
struct CompletePromise<void> {
    template <typename Closure>
    static void with(Promise<void>& promise, Closure&& closure) {
        closure();
        promise.tryToComplete();
    }
};

struct FutureTraits {
    template <typename Entity>
    struct For {
        template <typename... Args>
        struct WithArgs {
            using Result = decltype(std::declval<Entity>()(std::declval<Args>()...));
            using RefType = FutureRef<Result>;
            using ControlBlockType = detail::FutureControlBlock<Result>;

            static RefType devNullRef() {
                auto impl = std::shared_ptr<detail::FutureRefImpl<Result>>(
                    std::shared_ptr<detail::FutureRefImpl<Result>>(), &devNullFutureRef<Result>);
                return FutureRef<Result>(impl);
            }

            static RefType localRef(FiberSystem* system, ControlBlockType* block) {
                auto impl = std::make_shared<detail::LocalFutureRef<Result>>(system, block);
                return FutureRef<Result>(impl);
            }

            template <typename Runnable>
            static ControlBlockType*
            newControlBlock(const Path& path, std::unique_ptr<Mailbox> mailbox, Scheduler* bond, Runnable runnable) {
                auto block = new detail::FutureControlBlock<Result>;
                block->bound = bond;
                block->path = path;
                block->mailbox = std::move(mailbox);
                block->runnable = makeRunnable([runnable = std::move(runnable)] () mutable {
                    auto controlBlock = EventContext::current()->controlBlock();
                    auto futureControlBlock = static_cast<FutureControlBlock<Result>*>(controlBlock);

                    try {
                        CompletePromise<Result>::with(futureControlBlock->result, runnable);
                    } catch (...) {
                        futureControlBlock->result.tryToFail(std::current_exception());
                    }
                });
                return block;
            }
        };
    };
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_ENTITYTRAITS_HPP
