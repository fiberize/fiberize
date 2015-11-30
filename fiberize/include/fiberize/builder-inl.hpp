#ifndef FIBERIZE_BUILDERINL_HPP
#define FIBERIZE_BUILDERINL_HPP

#include <fiberize/builder.hpp>
#include <fiberize/fibersystem.hpp>

namespace fiberize {

namespace detail {

template <typename Entity, typename... Args>
auto bind(Entity&& entity, Args&&... args) {
    return std::bind(std::forward<Entity>(entity), std::forward<Args>(args)...);
}

template <typename Entity>
auto bind(Entity&& entity) {
    return std::forward<Entity>(entity);
}

} // namespace detail

template <typename TaskTraits, typename Entity, typename MailboxType>
template <typename... Args>
typename TaskTraits::template For<Entity>::template WithArgs<Args...>::RefType
Builder<TaskTraits, Entity, MailboxType>::run(Args&&... args) {
    using Traits = typename TaskTraits::template For<Entity>::template WithArgs<Args...>;

    FiberSystem* system = Scheduler::current()->system();
    if (!system->shuttingDown()) {
        /**
         * Create the task
         */
        Path path = PrefixedPath(system->uuid(), ident());
        std::unique_ptr<Mailbox> mailbox(new MailboxType(std::move(mailbox_)));
        auto task = Traits::newTask(std::move(path), std::move(mailbox), pin_,
            detail::bind<Entity, Args...>(std::move(task_), std::forward<Args>(args)...));
        task->grab();

        /**
         * Create the reference BEFORE starting the task. Otherwise the task could complete
         * before we can increment the refernce counter.
         */
        auto ref = Traits::localRef(system, task);

        /**
         * Schedule the task.
         */
        std::unique_lock<detail::TaskMutex> lock(task->mutex);
        context::detail::resume(task, std::move(lock));

        return ref;
    } else {
        return Traits::devNullRef();
    }
}

template <typename TaskTraits, typename Entity, typename MailboxType>
template <typename... Args>
void Builder<TaskTraits, Entity, MailboxType>::run_(Args&&... args) {
    using Traits = typename TaskTraits::template For<Entity>::template WithArgs<Args...>;

    FiberSystem* system = Scheduler::current()->system();
    if (!system->shuttingDown()) {
        /**
         * Create and schedule the task
         */
        Path path = PrefixedPath(system->uuid(), ident());
        std::unique_ptr<Mailbox> mailbox(new MailboxType(std::move(mailbox_)));
        auto task = Traits::newTask(std::move(path), std::move(mailbox), pin_,
            detail::bind<Entity, Args...>(std::move(task_), std::forward<Args>(args)...));
        task->grab();
        std::unique_lock<detail::TaskMutex> lock(task->mutex);
        context::detail::resume(task, std::move(lock));
    }
}

} // namespace fiberize

#endif // FIBERIZE_BUILDERINL_HPP
