#ifndef FIBERIZE_FIBERSYSTEM_HPP
#define FIBERIZE_FIBERSYSTEM_HPP

#include <utility>
#include <type_traits>

#include <boost/context/detail/fcontext.hpp>
#include <boost/type_traits.hpp>

#include <fiberize/promise.hpp>
#include <fiberize/builder.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/context.hpp>
#include <fiberize/detail/task.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>
#include <fiberize/detail/tasktraits.hpp>
#include <fiberize/detail/runner.hpp>
#include <fiberize/detail/singletaskscheduler.hpp>

namespace fiberize {

namespace detail {

class MultiTaskScheduler;

} // namespace detail

class FiberSystem {
public:
    /**
     * Starts the system with a number of macrothreads based on the number of cores.
     */
    FiberSystem();
    
    /**
     * Starts the system with the given number of macrothreads.
     */
    FiberSystem(uint32_t macrothreads);
    
    /**
     * Cleans up the main fiber.
     */
    ~FiberSystem();
    
    /**
     * Creates a new fiber builder using the given fiber instance and optionally a mailbox.
     * By default the fiber is unnamed, not pinned and has a DequeMailbox.
     */
    template <typename Fiber, typename MailboxType = DequeMailbox>
    Builder<detail::FiberTraits, Fiber, DequeMailbox>
    fiber(Fiber fiber, MailboxType mailbox = {}) {
        static_assert(std::is_move_constructible<Fiber>{}, "Fiber must be move constructible.");
        return Builder<detail::FiberTraits, Fiber, DequeMailbox>(
            boost::none,
            std::move(fiber),
            std::move(mailbox),
            nullptr,
            detail::runTaskAsMicrothread
        );
    }

    /**
     * Creates a new future builder using the given future instance and optionally a mailbox.
     * By default the future is unnamed, not pinned and has a DequeMailbox.
     */
    template <typename Future, typename MailboxType = DequeMailbox>
    Builder<detail::FutureTraits, Future, DequeMailbox>
    future(Future future, MailboxType mailbox = {}) {
        static_assert(std::is_move_constructible<Future>{}, "Future must be move constructible.");
        return Builder<detail::FutureTraits, Future, DequeMailbox>(
            boost::none,
            std::move(future),
            std::move(mailbox),
            nullptr,
            detail::runTaskAsMicrothread
        );
    }

    /**
     * Creates a new actor builder using the given actor instance and optionally a mailbox.
     * By default the actor is unnamed, not pinned and has a DequeMailbox.
     */
    template <typename Actor, typename MailboxType = DequeMailbox>
    Builder<detail::ActorTraits, Actor, DequeMailbox>
    actor(Actor actor, MailboxType mailbox = {}) {
        static_assert(std::is_move_constructible<Actor>{}, "Actor must be move constructible.");
        return Builder<detail::ActorTraits, Actor, DequeMailbox>(
            boost::none,
            std::move(actor),
            std::move(mailbox),
            nullptr,
            detail::runTaskAsMicrothread
        );
    }

    /**
     * Shut down the system.
     */
    void shutdown();

    /**
     * Whether the system is shutting down.
     */
    bool shuttingDown() const;

    /**
     * Returns the UUID of this system.
     */
    boost::uuids::uuid uuid() const;

    /**
     * Fiberize the current thread, enabling it to receive events.
     *
     * A thread once fiberized cannot be unfiberized. This makes the function leak some memory.
     * TODO: unfiberizing?
     */
    template <typename MailboxType = DequeMailbox>
    FiberRef fiberize(MailboxType mailbox = {}) {
        auto task = new detail::Task;
        task->pin = nullptr;
        task->path = PrefixedPath(uuid(), uniqueIdentGenerator.generate());
        task->mailbox.reset(new MailboxType(std::move(mailbox)));
        task->status = detail::Running;
        task->scheduled = false;

        std::uniform_int_distribution<uint64_t> seedDist;
        generatorMutex.lock();
        uint64_t seed = seedDist(seedGenerator);
        generatorMutex.unlock();

        auto scheduler = new detail::SingleTaskScheduler(this, seed, task);
        scheduler->makeCurrent();

        return FiberRef(std::make_shared<detail::LocalFiberRef>(this, task));
    }

    /**
     * Returns a vector of fiber schedulers.
     */
    inline const std::vector<detail::MultiTaskScheduler*>& schedulers() { return schedulers_; }

private:
    /**
     * Currently running schedulers.
     */
    std::vector<detail::MultiTaskScheduler*> schedulers_;
    
    /**
     * The prefix of this actor system.
     */
    boost::uuids::uuid uuid_;

    /**
     * Whether the system is shutting down.
     */
    bool shuttingDown_;

    // If valgrind support is enabled we cannot use std::random_device, because valgrind 3.11.0
    // doesn't recognize the rdrand instruction used in the implementation of random_device.
#ifdef FIBERIZE_VALGRIND
    std::default_random_engine seedGenerator;
#else
    std::random_device seedGenerator;
#endif

    std::mutex generatorMutex;
};
    
} // namespace fiberize

#endif // FIBERIZE_FIBERSYSTEM_HPP
