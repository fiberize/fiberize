#ifndef FIBERIZE_SYSTEM_HPP
#define FIBERIZE_SYSTEM_HPP

#include <utility>

#include <boost/context/all.hpp>

#include <fiberize/fiberref.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/executor.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>

namespace fiberize {

/**
 * Bundles the reference to a fiber and it's "finished" and "crashed" events.
 */
template <typename A>
class FiberResult {
public:
    FiberResult(const FiberRef &ref, const Event<A> &finished, const Event<Unit>& crashed)
        : ref_(ref), finished_(finished), crashed_(crashed) {}
    
    /**
     * Returns the reference to the fiber.
     */
    FiberRef ref() const {
        return ref_;
    }
    
    /**
     * An event which fires when the fiber successfully finishes.
     */
    Event<A> finished() const {
        return finished_;
    }
    
    /**
     * An event which fires when an uncaugh exception escapes the fiber run function.
     */
    Event<Unit> crashed() const {
        return crashed_;
    }
    
private:
    FiberRef ref_;
    Event<A> finished_;
    Event<Unit> crashed_;
};

class System {
public:
    /**
     * Starts the system with a number of macrothreads based on the number of cores.
     */
    System();
    
    /**
     * Starts the system with the given number of macrothreads.
     */
    System(uint32_t macrothreads);
    
    /**
     * Cleans up the main fiber.
     */
    ~System();
    
    /**
     * Starts a new unnamed fiber.
     * 
     * The fiber is constructed using the given arguments.
     */
    template <
        typename FiberImpl, 
        typename MailboxImpl = LockfreeQueueMailbox,
        typename Result = decltype(std::declval<FiberImpl>().run()),
        typename ...Args
        >
    FiberRef run(Args&& ...args) {
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
        typename MailboxImpl = LockfreeQueueMailbox,
        typename Result = decltype(std::declval<FiberImpl>().run()),
        typename ...Args
        >
    FiberRef runNamed(const std::string& name, Args&& ...args) {
        Event<Result> finished = newEvent<Result>();
        Event<Unit> crashed = newEvent<Unit>();
        return runImpl<MailboxImpl>(NamedIdent(name), [&] () {
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
     * Returns the fiber reference of the main thread.
     */
    FiberRef mainFiber();

    /**
     * Returns the context attached to the main thread.
     */
    Context* mainContext();

private:

    /**
     * Reschedule the fiber. It must be locked for writing.
     */
    void schedule(detail::ControlBlock* controlBlock);

    void fiberFinished();

    /**
     * Starts a new fiber.
     *
     * This is the most general of the "run" function family. It expects a factory used to create the
     * fiber and two events: one when the fiber finishes and the second when the fiber crashes.
     */
    template <
        typename MailboxImpl = LockfreeQueueMailbox,
        typename FiberFactory,
        typename FiberImpl = typename std::decay<decltype(*(std::declval<FiberFactory>()()))>::type,
        typename Result = decltype(std::declval<FiberImpl>().run())
        >
    FiberRef runImpl(const Ident& ident, const FiberFactory& fiberFactory) {
        std::shared_ptr<detail::FiberRefImpl> impl;
        if (!shuttingDown) {
            FiberImpl* fiber = fiberFactory();
            Event<Result> finished = newEvent<Result>();
            Event<Unit> crashed = newEvent<Unit>();

            // Create the control block.
            detail::ControlBlock* block = new detail::ControlBlock();
            block->path = PrefixedPath(uuid(), ident);
            block->mailbox.reset(new MailboxImpl());
            block->fiber.reset(fiber);
            block->finishedEventPath = finished.path();
            block->crashedEventPath = crashed.path();
            block->status = detail::Suspended;
            block->refCount = 0;
            block->executor = nullptr;

            // Increment fiber counter.
            std::atomic_fetch_add(&running, 1ul);

            // Schedule the block.
            block->mutex.lock();
            schedule(block);

            // Create a local reference.
            impl = std::make_shared<detail::LocalFiberRef>(this, block);
        } else {
            // System is shutting down, do not create new fibers.
            impl = std::make_shared<detail::DevNullFiberRef>();
        }
        return FiberRef(impl);
    }

    /**
     * Creates a block for a thread that is not running an executor.
     */
    template <typename MailboxImpl = LockfreeQueueMailbox>
    detail::ControlBlock* createUnmanagedBlock() {
        detail::ControlBlock* block = new detail::ControlBlock();
        block->path = PrefixedPath(uuid(), uniqueIdentGenerator.generate());
        block->mailbox.reset(new MailboxImpl());
        block->fiber.reset();
        block->finishedEventPath = DevNullPath();
        block->crashedEventPath = DevNullPath();
        block->status = detail::Running;
        block->refCount = 0;
        block->executor = nullptr;
        return block;
    }
    
    /**
     * Currently running executors.
     */
    std::vector<detail::Executor*> executors;
    
    bool shuttingDown;
    Event<Unit> allFibersFinished_;
    
    /**
     * Unmanaaged control block of the main thread.
     */
    detail::ControlBlock* mainControlBlock;
    
    /**
     * Context of the main thread.
     */
    Context mainContext_;

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
    
    friend detail::Executor;
    friend detail::LocalFiberRef;
};
    
} // namespace fiberize

#endif // FIBERIZE_SYSTEM_HPP
