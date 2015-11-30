/**
 * Implementation details of different task types.
 *
 * @file tasktraits.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_TASKTRAITS_HPP
#define FIBERIZE_DETAIL_TASKTRAITS_HPP

#include <utility>

#include <fiberize/path.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/detail/task.hpp>
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
            using TaskType = Task;

            inline static RefType devNullRef() {
                auto impl = std::shared_ptr<FiberRefImpl>(
                    std::shared_ptr<FiberRefImpl>(), &devNullFiberRef);
                return FiberRef(impl);
            }

            inline static RefType localRef(FiberSystem* system, TaskType* task) {
                auto impl = std::make_shared<LocalFiberRef>(system, task);
                return FiberRef(impl);
            }

            template <typename Runnable>
            inline static TaskType*
            newTask(const Path& path, std::unique_ptr<Mailbox> mailbox, Scheduler* pin, Runnable runnable) {
                auto task = new Task;
                task->pin = pin;
                task->path = path;
                task->mailbox = std::move(mailbox);
                task->runnable = makeRunnable([runnable = std::move(runnable)] () mutable {
                    try {
                        runnable();
                    } catch (...) {
                        // Nothing.
                    }

                    context::detail::terminate();
                });
                return task;
            }
        };
    };
};


template <typename A>
struct CompletePromise {
    template <typename Closure>
    static void with(Promise<A>& promise, Closure& closure) {
        promise.tryToComplete(closure());
    }
};

template <>
struct CompletePromise<void> {
    template <typename Closure>
    static void with(Promise<void>& promise, Closure& closure) {
        closure();
        promise.complete();
    }
};

struct FutureTraits {
    template <typename Entity>
    struct For {
        template <typename... Args>
        struct WithArgs {
            using A = decltype(std::declval<Entity>()(std::declval<Args>()...));
            using RefType = FutureRef<A>;
            using TaskType = Future<A>;

            static RefType devNullRef() {
                auto impl = std::shared_ptr<FutureRefImpl<A>>(
                    std::shared_ptr<FutureRefImpl<A>>(), &devNullFutureRef<A>);
                return FutureRef<A>(impl);
            }

            static RefType localRef(FiberSystem* system, TaskType* task) {
                auto impl = std::make_shared<LocalFutureRef<A>>(system, task);
                return FutureRef<A>(impl);
            }

            template <typename Runnable>
            static TaskType*
            newTask(const Path& path, std::unique_ptr<Mailbox> mailbox, Scheduler* pin, Runnable runnable) {
                auto task = new Future<A>;
                task->pin = pin;
                task->path = path;
                task->mailbox = std::move(mailbox);
                task->runnable = makeRunnable([runnable = std::move(runnable)] () mutable {
                    auto task = context::detail::task();
                    auto future = static_cast<Future<A>*>(task);
                    future->result.complete(result(runnable));
                    context::detail::terminate();
                });
                return task;
            }
        };
    };
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_TASKTRAITS_HPP
