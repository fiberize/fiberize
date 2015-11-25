#ifndef FIBERIZE_FIBERSYSTEM_HPP
#define FIBERIZE_FIBERSYSTEM_HPP

#include <utility>
#include <type_traits>

#include <boost/context/all.hpp>

#include <fiberize/fiber.hpp>
#include <fiberize/future.hpp>
#include <fiberize/promise.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>
#include <fiberize/detail/threadscheduler.hpp>

namespace fiberize {

namespace detail {

class FiberScheduler;

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
     * Starts a new unnamed fiber.
     * 
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl, 
        typename MailboxImpl = DequeMailbox,
        typename std::enable_if<
             std::is_base_of<Fiber, FiberImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    FiberRef run(Args&& ...args) {
        return runFiber<MailboxImpl>(uniqueIdentGenerator.generate(), false, [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed future.
     *
     * The future is constructed using the given arguments.
     */
    template <
        typename FutureImpl,
        typename MailboxImpl = DequeMailbox,
        typename Result = decltype(std::declval<FutureImpl>().run()),
        typename std::enable_if<
             std::is_base_of<Future<Result>, FutureImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    FutureRef<Result> run(Args&& ...args) {
        return runFuture<MailboxImpl>(uniqueIdentGenerator.generate(), false, [&] () {
            return new FutureImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed fiber. This version doesn't return the fiber reference.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = DequeMailbox,
        typename std::enable_if<
             std::is_base_of<Fiber, FiberImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    void run_(Args&& ...args) {
        runFiber_<MailboxImpl>(uniqueIdentGenerator.generate(), false, [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed future. This version doesn't return the future reference.
     *
     * The future is constructed using the given arguments.
     */
    template <
        typename FutureImpl,
        typename MailboxImpl = DequeMailbox,
        typename Result = decltype(std::declval<FutureImpl>().run()),
        typename std::enable_if<
             std::is_base_of<Future<Result>, FutureImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    void run_(Args&& ...args) {
        runFuture_<MailboxImpl>(uniqueIdentGenerator.generate(), false, [&] () {
            return new FutureImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed fiber, bound to this scheduler.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = DequeMailbox,
        typename std::enable_if<
             std::is_base_of<Fiber, FiberImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    FiberRef runBound(Args&& ...args) {
        return runFiber<MailboxImpl>(uniqueIdentGenerator.generate(), true, [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed future, bound to this scheduler.
     *
     * The future is constructed using the given arguments.
     */
    template <
        typename FutureImpl,
        typename MailboxImpl = DequeMailbox,
        typename Result = decltype(std::declval<FutureImpl>().run()),
        typename std::enable_if<
             std::is_base_of<Future<Result>, FutureImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    FutureRef<Result> runBound(Args&& ...args) {
        return runFuture<MailboxImpl>(uniqueIdentGenerator.generate(), true, [&] () {
            return new FutureImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed fiber, bound to this scheduler. This version doesn't return the fiber reference.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = DequeMailbox,
        typename std::enable_if<
             std::is_base_of<Fiber, FiberImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    void runBound_(Args&& ...args) {
        runFiber_<MailboxImpl>(uniqueIdentGenerator.generate(), true, [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed future, bound to this scheduler. This version doesn't return the future reference.
     *
     * The future is constructed using the given arguments.
     */
    template <
        typename FutureImpl,
        typename MailboxImpl = DequeMailbox,
        typename Result = decltype(std::declval<FutureImpl>().run()),
        typename std::enable_if<
             std::is_base_of<Future<Result>, FutureImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    void runBound_(Args&& ...args) {
        runFuture_<MailboxImpl>(uniqueIdentGenerator.generate(), true, [&] () {
            return new FutureImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new named fiber.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = DequeMailbox,
        typename std::enable_if<
             std::is_base_of<Fiber, FiberImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    FiberRef runNamed(const std::string& name, Args&& ...args) {
        return runFiber<MailboxImpl>(Ident(NamedIdent(name)), false, [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new named future.
     *
     * The future is constructed using the given arguments.
     */
    template <
        typename FutureImpl,
        typename MailboxImpl = DequeMailbox,
        typename Result = decltype(std::declval<FutureImpl>().run()),
        typename std::enable_if<
             std::is_base_of<Future<Result>, FutureImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    FutureRef<Result> runNamed(const std::string& name, Args&& ...args) {
        return runFuture<MailboxImpl>(Ident(NamedIdent(name)), false, [&] () {
            return new FutureImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new named fiber. This version doesn't return the fiber reference.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = DequeMailbox,
        typename std::enable_if<
             std::is_base_of<Fiber, FiberImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    void runNamed_(const std::string& name, Args&& ...args) {
        runFiber_<MailboxImpl>(Ident(NamedIdent(name)), false, [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new named future. This version doesn't return the future reference.
     *
     * The future is constructed using the given arguments.
     */
    template <
        typename FutureImpl,
        typename MailboxImpl = DequeMailbox,
        typename Result = decltype(std::declval<FutureImpl>().run()),
        typename std::enable_if<
             std::is_base_of<Future<Result>, FutureImpl>{}
            >::type* = nullptr,
        typename ...Args
        >
    void runNamed_(const std::string& name, Args&& ...args) {
        runFuture_<MailboxImpl>(Ident(NamedIdent(name)), false, [&] () {
            return new FutureImpl(std::forward<Args>(args)...);
        });
    }
    
    /**
     * Shut down the system.
     */
    void shutdown();

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
    template <typename MailboxImpl = DequeMailbox>
    FiberRef fiberize() {
        auto controlBlock = createThreadControlBlock<MailboxImpl>();
        controlBlock->eventContext = new EventContext(this, controlBlock);
        controlBlock->eventContext->makeCurrent();

        // TODO: real seed
        auto scheduler = new detail::ThreadScheduler(this, 123, controlBlock);
        scheduler->makeCurrent();

        /**
         * Bind the thread's control block to the thread's scheduler.
         */
        controlBlock->bound = scheduler;

        return FiberRef(std::make_shared<detail::LocalFiberRef>(this, controlBlock));
    }

    /**
     * Returns a vector of fiber schedulers.
     */
    inline const std::vector<detail::FiberScheduler*>& schedulers() { return schedulers_; }

private:
    /**
     * Starts a new fiber, using a factory to construct it.
     */
    template <
        typename MailboxImpl = DequeMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type
        >
    FiberRef runFiber(const Ident& ident, bool bound, const FiberFactory& fiberFactory) {
        std::shared_ptr<detail::FiberRefImpl> impl;
        if (!shuttingDown) {
            auto block = createFiberControlBlock(ident, fiberFactory);
            if (bound) block->bound = Scheduler::current();

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
            Scheduler::current()->enableFiber(block, std::move(lock));

            // Create a local reference.
            impl = std::make_shared<detail::LocalFiberRef>(this, block);
        } else {
            // System is shutting down, do not create new fibers.
            impl = std::make_shared<detail::DevNullFiberRef>();
        }
        return FiberRef(impl);
    }

    /**
     * Starts a new fiber, using a factory to construct it. Does not return the reference.
     */
    template <
        typename MailboxImpl = DequeMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type
        >
    void runFiber_(const Ident& ident, bool bound, const FiberFactory& fiberFactory) {
        if (!shuttingDown) {
            auto block = createFiberControlBlock(ident, fiberFactory);
            if (bound) block->bound = Scheduler::current();

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
            Scheduler::current()->enableFiber(block, std::move(lock));
        } else {
            // System is shutting down, do not create new fibers.
        }
    }

    /**
     * Starts a new future, using a factory to construct it.
     */
    template <
        typename MailboxImpl = DequeMailbox,
        typename FutureFactory,
        typename FutureImpl = typename std::decay<decltype(*(std::declval<FutureFactory>()()))>::type,
        typename Result = decltype(std::declval<FutureImpl>().run())
        >
    FutureRef<Result> runFuture(const Ident& ident, bool bound, const FutureFactory& futureFactory) {
        std::shared_ptr<detail::FutureRefImpl<Result>> impl;
        if (!shuttingDown) {
            auto block = createFutureControlBlock(ident, futureFactory);
            if (bound) block->bound = Scheduler::current();

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
            Scheduler::current()->enableFiber(block, std::move(lock));

            // Create a local reference.
            impl = std::make_shared<detail::LocalFutureRef<Result>>(this, block);
        } else {
            // System is shutting down, do not create new fibers.
            impl = std::make_shared<detail::DevNullFutureRef<Result>>();
        }
        return FutureRef<Result>(impl);
    }

    /**
     * Starts a new future, using a factory to construct it. Does not return the reference.
     */
    template <
        typename MailboxImpl = DequeMailbox,
        typename FutureFactory,
        typename FutureImpl = typename std::decay<decltype(*(std::declval<FutureFactory>()()))>::type
        >
    void runFuture_(const Ident& ident, bool bound, const FutureFactory& futureFactory) {
        if (!shuttingDown) {
            auto block = createFutureControlBlock(ident, futureFactory);
            if (bound) block->bound = Scheduler::current();

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
            Scheduler::current()->enableFiber(block, std::move(lock));
        } else {
            // System is shutting down, do not create new fibers.
        }
    }

    /**
     * Creates a control block for a fiber.
     */
    template <
        typename MailboxImpl = DequeMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type
        >
    detail::FiberControlBlock* createFiberControlBlock(const Ident& ident, const FiberFactory& fiberFactory) {
        FiberImpl* fiber = fiberFactory();

        // Create the control block.
        auto block = new detail::FiberControlBlock;
        block->refCount = 1;
        block->bound = nullptr;
        block->path = PrefixedPath(uuid(), ident);
        block->mailbox.reset(new MailboxImpl);
        block->runnable.reset(fiber);
        block->status = detail::Suspended;
        block->reschedule = false;
        return block;
    }

    /**
     * Creates a control block for a future.
     */
    template <
        typename MailboxImpl = DequeMailbox,
        typename FutureFactory,
        typename FutureImpl = typename std::decay<decltype(*(std::declval<FutureFactory>()()))>::type,
        typename Result = decltype(std::declval<FutureImpl>().run())
        >
    detail::FutureControlBlock<Result>* createFutureControlBlock(const Ident& ident, const FutureFactory& futureFactory) {
        FutureImpl* future = futureFactory();

        // Create the control block.
        auto block = new detail::FutureControlBlock<Result>;
        block->refCount = 1;
        block->bound = nullptr;
        block->path = PrefixedPath(uuid(), ident);
        block->mailbox.reset(new MailboxImpl);
        block->runnable.reset(future);
        block->status = detail::Suspended;
        block->reschedule = false;
        return block;
    }

    /**
     * Creates a block for a thread that is not running an executor.
     */
    template <typename MailboxImpl = DequeMailbox>
    detail::ThreadControlBlock* createThreadControlBlock() {
        auto block = new detail::ThreadControlBlock;
        block->refCount = 1;
        block->bound = nullptr;
        block->path = PrefixedPath(uuid(), uniqueIdentGenerator.generate());
        block->mailbox.reset(new MailboxImpl);
        block->status = detail::Running;
        return block;
    }
    
    /**
     * Currently running schedulers.
     */
    std::vector<detail::FiberScheduler*> schedulers_;
    
    /**
     * The prefix of this actor system.
     */
    boost::uuids::uuid uuid_;

    /**
     * Number of running actors.
     */
    std::atomic<uint64_t> running;

    bool shuttingDown;
};
    
} // namespace fiberize

#endif // FIBERIZE_FIBERSYSTEM_HPP
