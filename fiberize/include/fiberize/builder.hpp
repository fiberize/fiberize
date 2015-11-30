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
public:
    /**
     * @name Construction and copying
     */
    ///@{

    /**
     * Initializes a builder with the given values.
     */
    Builder(
        boost::optional<std::string> name,
        TaskType task,
        MailboxType mailbox,
        Scheduler* pin)
        : name_(std::move(name))
        , task_(std::move(task))
        , mailbox_(std::move(mailbox))
        , pin_(pin)
        {}

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
        return name_;
    }

    /**
     * Returns the task.
     */
    TaskType& task() const {
        return task_;
    }

    /**
     * Returns the mailbox.
     */
    MailboxType& mailbox() const {
        return mailbox_;
    }

    /**
     * Scheduler the entities built by this builder are pinned to.
     */
    Scheduler*& pin() const {
        return pin_;
    }

    ///@}

    /**
     * @name Modyfing the builder.
     *
     * Methods in this section are used to modify the parameters of the runnable task.
     * Instead of modyfing the builder directly, they return a new one, with old variables
     * moved (instead of copied) to the new one. This invalidates the old builder. If you
     * want to use a builder more then once, you have to copy() it.
     */
    ///@{

    /**
     * Creates a new builder that is going to build a task pinned to the currently running scheduler.
     * @warning This invalidates the current builder.
     */
    Builder pinned() {
        return Builder(std::move(name_), std::move(task_), std::move(mailbox_), Scheduler::current());
    }

    /**
     * Creates a new builder that is going to build a task pinned to the the given scheduler.
     * @warning This invalidates the current builder.
     */
    Builder pinned(Scheduler* scheduler) {
        return Builder(std::move(name_), std::move(task_), std::move(mailbox_), scheduler);
    }

    /**
     * Creates a new builder that is going to build a task not pinned to any scheduler.
     * @warning This invalidates the current builder.
     */
    Builder detached() const {
        return Builder(std::move(name_), std::move(task_), std::move(mailbox_), nullptr);
    }

    /**
     * Creates a new builder that is going to build a named task.
     * @warning This invalidates the current builder.
     */
    Builder named(std::string name) const {
        return Builder(std::move(name), std::move(task_), std::move(mailbox_), pin_);
    }

    /**
     * Creates a new builder that is going to build an unnamed task.
     * @warning This invalidates the current builder.
     */
    Builder unnamed() const {
        return Builder(boost::none_t{}, std::move(task_), std::move(mailbox_), pin_);
    }

    /**
     * Creates a new builder that is going to build a task with the given mailbox.
     * @warning This invalidates the current builder.
     */
    template <typename NewMailboxType>
    Builder<TaskTraits, TaskType, NewMailboxType> withMailbox(std::unique_ptr<NewMailboxType> newMailbox) const {
        static_assert(boost::is_base_of<Mailbox, MailboxType>::value_type,
            "The given mailbox type must be derived from Maiblox.");
        mailbox_.reset();
        return Builder<TaskTraits, TaskType, NewMailboxType>(
            std::move(name_), std::move(task_), std::move(newMailbox), pin_
        );
    }

    ///@}

    /**
     * @name Running
     */
    ///@{

    /**
     * Runs the task using the given arguments and returns a reference.
     * @warning This invalidates the builder.
     * @warning This must be executed on a thread with an attached scheduler.
     */
    template <typename... Args>
    typename TaskTraits::template For<TaskType>::template WithArgs<Args...>::RefType
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

    boost::optional<std::string> name_;
    TaskType task_;
    MailboxType mailbox_;
    Scheduler* const pin_;
};

} // namespace fiberize

#endif // FIBERIZE_BUILDER_HPP
