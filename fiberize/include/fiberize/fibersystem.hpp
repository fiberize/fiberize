#ifndef FIBERIZE_FIBERSYSTEM_HPP
#define FIBERIZE_FIBERSYSTEM_HPP

#include <utility>
#include <type_traits>

#include <boost/context/all.hpp>
#include <boost/type_traits.hpp>

#include <fiberize/fiber.hpp>
#include <fiberize/future.hpp>
#include <fiberize/promise.hpp>
#include <fiberize/builder.hpp>
#include <fiberize/fiberref.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/eventcontext.hpp>
#include <fiberize/detail/controlblock.hpp>
#include <fiberize/detail/localfiberref.hpp>
#include <fiberize/detail/devnullfiberref.hpp>
#include <fiberize/detail/threadscheduler.hpp>
#include <fiberize/detail/entitytraits.hpp>

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
     * Creates a new builder with default configuration, that will construct a fiber
     * of the specified type.
     */
    template <typename FiberType>
    Builder<FiberType, detail::FiberTraits> fiber() {
        static_assert(boost::is_base_of<Fiber, FiberType>{},
            "The specified type must be derived from Fiber.");
        return Builder<FiberType, detail::FiberTraits>();
    }

    /**
     * Creates a new builder with default configuration, that will construct a future
     * of the specified type.
     */
    template <typename FutureType, typename Result = decltype(std::declval<FutureType>().run())>
    Builder<FutureType, detail::FutureTraits<Result>> future() {
        static_assert(boost::is_base_of<Future<Result>, FutureType>{},
            "The specified type must be derived from Future.");
        return Builder<FutureType, detail::FutureTraits<Result>>();
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
    template <typename MailboxImpl = DequeMailbox>
    FiberRef fiberize() {
        auto controlBlock = createThreadControlBlock<MailboxImpl>();
        controlBlock->eventContext = new EventContext(this, controlBlock);
        controlBlock->eventContext->makeCurrent();

        std::uniform_int_distribution<uint64_t> seedDist;
        generatorMutex.lock();
        uint64_t seed = seedDist(seedGenerator);
        generatorMutex.unlock();

        auto scheduler = new detail::ThreadScheduler(this, seed, controlBlock);
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
