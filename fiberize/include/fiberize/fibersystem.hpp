#ifndef FIBERIZE_FIBERSYSTEM_HPP
#define FIBERIZE_FIBERSYSTEM_HPP

#include <utility>

#include <boost/context/all.hpp>

#include <fiberize/promise.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/fibercontext.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace fiberize {

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
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename Result = decltype(std::declval<FiberImpl>().run()),
        typename ...Args
        >
    FiberRef<Result> run(Args&& ...args) {
        return runImpl<MailboxImpl>(uniqueIdentGenerator.generate(), [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new named fiber.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename Result = decltype(std::declval<FiberImpl>().run()),
        typename ...Args
        >
    FiberRef<Result> runNamed(const std::string& name, Args&& ...args) {
        return runImpl<MailboxImpl>(NamedIdent(name), [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new unnamed fiber.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename Result = decltype(std::declval<FiberImpl>().run()),
        typename ...Args
        >
    void run_(Args&& ...args) {
        runImpl_<MailboxImpl>(uniqueIdentGenerator.generate(), [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Starts a new named fiber.
     *
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl,
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename Result = decltype(std::declval<FiberImpl>().run()),
        typename ...Args
        >
    void runNamed_(const std::string& name, Args&& ...args) {
        runImpl_<MailboxImpl>(NamedIdent(name), [&] () {
            return new FiberImpl(std::forward<Args>(args)...);
        });
    }

    /**
     * Creates a new unique event.
     */
    template <typename A>
    Event<A> newEvent() {
        return Event<A>::fromPath(PrefixedPath(uuid(), uniqueIdentGenerator.generate()));
    }
    
    /**
     * Shut down the system.
     */
    void shutdown();

    /**
     * Event that fires when all fibers finished running.
     */
    Event<Unit> allFibersFinished();
    
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
    template <typename MailboxImpl = MoodyCamelConcurrentQueueMailbox>
    AnyFiberRef fiberize() {
        detail::ControlBlockPtr controlBlock = createUnmanagedBlock<MailboxImpl>();
        (new FiberContext(this, controlBlock))->makeCurrent();
        return AnyFiberRef(std::make_shared<detail::LocalFiberRef>(this, controlBlock));
    }

    /**
     * Subscribes to events. TODO: refactor
     */
    void subscribe(AnyFiberRef ref);

private:

    /**
     * Reschedule the fiber. It must be locked for writing.
     */
    void schedule(detail::ControlBlockPtr controlBlock, boost::unique_lock<detail::ControlBlockMutex>&& lock);

    void fiberFinished();

    /**
     * Starts a new fiber.
     *
     * This is the most general of the "run" function family. It expects a factory used to create the
     * fiber and two events: one when the fiber finishes and the second when the fiber crashes.
     */
    template <
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type,
        typename Result = decltype(std::declval<FiberImpl>().run())
        >
    FiberRef<Result> runImpl(const Ident& ident, const FiberFactory& fiberFactory) {
        std::shared_ptr<detail::FiberRefImpl> impl;
        if (!shuttingDown) {
            auto block = createManagedBlock(ident, fiberFactory);

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
            schedule(block, std::move(lock));

            // Create a local reference.
            impl = std::make_shared<detail::LocalFiberRef>(this, std::move(block));
        } else {
            // System is shutting down, do not create new fibers.
            impl = std::make_shared<detail::DevNullFiberRef>();
        }
        return FiberRef<Result>(impl);
    }

    /**
     * Starts a new fiber, without creating a reference.
     *
     * This is the most general of the "run" function family. It expects a factory used to create the
     * fiber and two events: one when the fiber finishes and the second when the fiber crashes.
     */
    template <
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type,
        typename Result = decltype(std::declval<FiberImpl>().run())
        >
    void runImpl_(const Ident& ident, const FiberFactory& fiberFactory) {
        if (!shuttingDown) {
            auto block = createManagedBlock(ident, fiberFactory);

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            boost::unique_lock<detail::ControlBlockMutex> lock(block->mutex);
            schedule(std::move(block), std::move(lock));
        } else {
            // System is shutting down, do not create new fibers.
        }
    }

    /**
     * Creates a new managed block.
     */
    template <
        typename MailboxImpl = MoodyCamelConcurrentQueueMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type,
        typename Result = decltype(std::declval<FiberImpl>().run())
        >
    detail::ControlBlockPtr createManagedBlock(const Ident& ident, const FiberFactory& fiberFactory) {
        FiberImpl* fiber = fiberFactory();
        Event<Result> finished = newEvent<Result>();
        Event<Unit> crashed = newEvent<Unit>();

        // Create the control block.
        detail::ControlBlock* block = alllocateBlock();
        block->refCount = 0;
        block->path = PrefixedPath(uuid(), ident);
        block->mailbox = MailboxPool<MailboxImpl>::current.allocate();
        block->fiber.reset(fiber);
        block->result.reset(new Promise<Result>(newEvent<Unit>()));
        block->status = detail::Suspended;
        block->reschedule = false;
        return detail::ControlBlockPtr(block);
    }

    detail::ControlBlock* alllocateBlock() {
        return new detail::ControlBlock();
/*        auto memory = controlBlockAllocator.allocate(1);
        new (memory) detail::ControlBlock;
        return detail::ControlBlockPtr(
            memory,
            [] (detail::ControlBlock* ptr) {
                ptr->~ControlBlock();
                controlBlockAllocator.deallocate(ptr, 1);
            }
        );*/
    }

    /**
     * Creates a block for a thread that is not running an executor.
     */
    template <typename MailboxImpl = MoodyCamelConcurrentQueueMailbox>
    detail::ControlBlockPtr createUnmanagedBlock() {
        auto block = alllocateBlock();
        block->refCount = 0;
        block->path = PrefixedPath(uuid(), uniqueIdentGenerator.generate());
        block->mailbox = MailboxPool<MailboxImpl>::current.allocate();
        block->fiber.reset();
        block->result.reset(new Promise<Void>(newEvent<Unit>()));
        block->status = detail::Running;
        block->reschedule = false;
        return detail::ControlBlockPtr(block);
    }
    
    /**
     * Currently running executors.
     */
    std::vector<detail::Executor*> executors;
    
    /**
     * The prefix of this actor system.
     */
    boost::uuids::uuid uuid_;

    /**
     * Number of running actors.
     */
    std::atomic<uint64_t> running;

    /**
     * Counter used implement round robin task balancing.
     */
    std::atomic<uint64_t> roundRobinCounter;
    
    /**
     * Generator used for event and fiber ids.
     */
    UniqueIdentGenerator uniqueIdentGenerator;

    static boost::fast_pool_allocator<detail::ControlBlock> controlBlockAllocator;

    bool shuttingDown;

    // Events, TODO: refactor
    std::mutex subscribersMutex;
    std::vector<AnyFiberRef> subscribers;
    Event<Unit> allFibersFinished_;
    
    friend detail::Executor;
    friend detail::LocalFiberRef;
};
    
} // namespace fiberize

#endif // FIBERIZE_FIBERSYSTEM_HPP
