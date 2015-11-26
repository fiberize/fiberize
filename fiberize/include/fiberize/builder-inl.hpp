#ifndef FIBERIZE_BUILDERINL_HPP
#define FIBERIZE_BUILDERINL_HPP

#include <fiberize/builder.hpp>
#include <fiberize/fibersystem.hpp>

namespace fiberize {

template <typename Entity, typename EntityTraits>
template <typename... Args>
typename EntityTraits::RefType Builder<Entity, EntityTraits>::run(Args&&... args) const {
    FiberSystem* system = Scheduler::current()->system();
    if (!system->shuttingDown()) {
        /**
         * Prepare the arguments.
         */
        Path path = PrefixedPath(system->uuid(), ident());
        std::unique_ptr<Mailbox> mailbox(mailboxFactory_());
        std::unique_ptr<Runnable> runnable(new Entity(std::forward<Args>(args)...));

        /**
         * Create an schedule the block
         */
        auto block = EntityTraits::newControlBlock(path, bond(), std::move(mailbox), std::move(runnable));
        boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
        Scheduler::current()->enableFiber(block, std::move(lock));

        /**
         * Create the reference.
         */
        return EntityTraits::localRef(system, block);
    } else {
        return EntityTraits::devNullRef();
    }
}

template <typename Entity, typename EntityTraits>
template <typename... Args>
void Builder<Entity, EntityTraits>::run_(Args&&... args) const {
    FiberSystem* system = Scheduler::current()->system();
    if (!system->shuttingDown()) {
        /**
         * Prepare the arguments.
         */
        Path path = PrefixedPath(system->uuid(), ident());
        std::unique_ptr<Mailbox> mailbox(mailboxFactory_());
        std::unique_ptr<Runnable> runnable(new Entity(std::forward<Args>(args)...));

        /**
         * Create an schedule the block.
         */
        auto block = EntityTraits::newControlBlock(path, bond(), std::move(mailbox), std::move(runnable));
        boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
        Scheduler::current()->enableFiber(block, std::move(lock));
    }
}

} // namespace fiberize

#endif // FIBERIZE_BUILDERINL_HPP
