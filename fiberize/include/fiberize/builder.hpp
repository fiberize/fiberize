/**
 * Builder used to construct and run tasks.
 *
 * @file builder.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_BUILDER_HPP
#define FIBERIZE_BUILDER_HPP

#include <boost/type_traits.hpp>
#include <boost/optional.hpp>

#include <fiberize/path.hpp>
#include <fiberize/scheduler.hpp>
#include <fiberize/detail/runner.hpp>

namespace fiberize {

class Mailbox;

/**
 * Task builder.
 *
 * A builder class used to configure tasks, like fibers, futures, threads or actors.
 * After configuration you can start the task with the run (or run_) function.
 */
template <typename TaskTraits, typename TaskType, typename MailboxType>
class Builder {
private:
    /**
     * Initializes a builder with the given values.
     */
    Builder(
        boost::optional<std::string> name,
        TaskType task,
        MailboxType mailbox,
        Scheduler* pin,
        void (*runner)(detail::Task*))
        : invalidated(false)
        , name_(std::move(name))
        , task_(std::move(task))
        , mailbox_(std::move(mailbox))
        , pin_(pin)
        , runner_(runner)
        {}

    friend class FiberSystem;

public:
    /**
     * @name Construction and copying
     */
    ///@{

    /**
     * Copies a builder.
     */
    Builder(const Builder&) = default;

    /**
     * Copies a builder.
     */
    Builder& operator = (const Builder&) = default;

    /**
     * Moves a builder.
     */
    Builder(Builder&&) = default;

    /**
     * Moves a builder.
     */
    Builder& operator = (Builder&&) = default;

    /**
     * Returns a copy of this builder.
     */
    Builder copy() const {
        assert(!invalidated);
        return Builder(*this);
    }

    ///@}

    /**
     * @name Accessing fields
     */
    ///@{

    /**
     * The name of the entities built by this builder, if they are named.
     */
    boost::optional<std::string>& name() const {
        assert(!invalidated);
        return name_;
    }

    /**
     * Returns the task.
     */
    TaskType& task() const {
        assert(!invalidated);
        return task_;
    }

    /**
     * Returns the mailbox.
     */
    MailboxType& mailbox() const {
        assert(!invalidated);
        return mailbox_;
    }

    /**
     * Scheduler the entities built by this builder are pinned to.
     */
    Scheduler*& pin() const {
        assert(!invalidated);
        return pin_;
    }

    ///@}

    /**
     * @name Modyfing the builder.
     *
     * Methods in this section are used to modify the parameters of the task.
     */
    ///@{

    /**
     * Pins the task to the currently running scheduler.
     */
    Builder& pinned() {
        assert(!invalidated);
        pin_ = Scheduler::current();
        return *this;
    }

    /**
     * Pins the task to the given scheduler.
     */
    Builder& pinned(Scheduler* scheduler) {
        assert(!invalidated);
        pin_ = scheduler;
        return *this;
    }

    /**
     * Unpins the task.
     * @note This is the default.
     */
    Builder& detached() const {
        assert(!invalidated);
        pin_ = nullptr;
        return *this;
    }

    /**
     * Names the task.
     */
    Builder& named(std::string name) const {
        assert(!invalidated);
        name_ = name;
        return *this;
    }

    /**
     * Makes the task unnamed.
     * @note This is the default.
     */
    Builder& unnamed() const {
        assert(!invalidated);
        name_ = boost::none;
        return *this;
    }

    /**
     * Configures the task to execute as a microthread.
     * @note This is the default.
     */
    Builder& microthread() {
        assert(!invalidated);
        runner_ = detail::runTaskAsMicrothread;
        return *this;
    }

    /**
     * Configures the task to execute as an OS thread.
     * @note This overrides the "pinned" setting.
     */
    Builder& osthread() {
        assert(!invalidated);
        runner_ = detail::runTaskAsOSThread;
        return *this;
    }

    ///@}

    /**
     * @name Running
     */
    ///@{

    template <typename... Args>
    using TraitsFor = typename TaskTraits::template ForResult<decltype(std::declval<TaskType>()(std::declval<Args>()...))>;

    /**
     * Runs the task using the given arguments and returns a reference.
     * @returns A FiberRef or FutureRef, depending on the task type.
     * @warning This invalidates the builder.
     * @warning This must be executed on a thread with an attached scheduler.
     */
    template <typename... Args>
    typename TraitsFor<Args...>::RefType
    run(Args&&... args);

    /**
     * Runs the task using the given arguments.
     * @warning This invalidates the builder.
     * @warning This must be executed on a thread with an attached scheduler.
     */
    template <typename... Args>
    void run_(Args&&... args);

    ///@}

private:
    /**
     * Creates an identifier from this builder's name. If the name is not
     * set then this returns an newly generated unique id.
     * @warning Moves the name.
     * @internal
     */
    Ident ident() const {
        if (name_.is_initialized()) {
            return std::move(name_.get());
        } else {
            return uniqueIdentGenerator.generate();
        }
    }

    bool invalidated;
    boost::optional<std::string> name_;
    TaskType task_;
    MailboxType mailbox_;
    Scheduler* const pin_;
    void (*runner_)(detail::Task*);
};

} // namespace fiberize

#endif // FIBERIZE_BUILDER_HPP
