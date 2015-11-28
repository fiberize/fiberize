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

template <typename EntityTraits, typename Entity, typename MailboxType>
template <typename... Args>
typename EntityTraits::template For<Entity>::template WithArgs<Args...>::RefType
Builder<EntityTraits, Entity, MailboxType>::run(Args&&... args) {
    using Traits = typename EntityTraits::template For<Entity>::template WithArgs<Args...>;

    FiberSystem* system = Scheduler::current()->system();
    if (!system->shuttingDown()) {
        /**
         * Create an schedule the block
         */
        Path path = PrefixedPath(system->uuid(), ident());
        std::unique_ptr<Mailbox> mailbox(new MailboxType(std::move(mailbox_)));
        auto block = Traits::newControlBlock(std::move(path), std::move(mailbox), bond_,
            detail::bind<Entity, Args...>(std::move(entity_), std::forward<Args>(args)...));
        boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
        Scheduler::current()->enableFiber(block, std::move(lock));

        /**
         * Create the reference.
         */
        return Traits::localRef(system, block);
    } else {
        return Traits::devNullRef();
    }
}

template <typename EntityTraits, typename Entity, typename MailboxType>
template <typename... Args>
void Builder<EntityTraits, Entity, MailboxType>::run_(Args&&... args) {
    using Traits = typename EntityTraits::template For<Entity>::template WithArgs<Args...>;

    FiberSystem* system = Scheduler::current()->system();
    if (!system->shuttingDown()) {
        /**
         * Prepare the arguments.
         */
        Path path = PrefixedPath(system->uuid(), ident());
        std::unique_ptr<Mailbox> mailbox(new MailboxType(std::move(mailbox_)));
        auto block = Traits::newControlBlock(std::move(path), std::move(mailbox), bond_,
            detail::bind<Entity, Args...>(std::move(entity_), std::forward<Args>(args)...));
        boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
        Scheduler::current()->enableFiber(block, std::move(lock));
    }
}

} // namespace fiberize

#endif // FIBERIZE_BUILDERINL_HPP
