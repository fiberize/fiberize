/**
 * Builder used to construct and run fibers and futures.
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
 * Runnable entity builder.
 *
 * A builder class used to configure runnable entities, like fibers,
 * futures, threads or actors. After configuration you can start the
 * entity with the run (or run_) function.
 */
template <typename Entity, typename EntityTraits>
class Builder {
public:
    static_assert(boost::is_base_of<Runnable, Entity>{},
        "The constructed entity type must be derived from Runnable.");

    /**
     * Initializes the builder with default values.
     *
     * By default the constructed entities are unnamed and unbound.
     * The default mailbox is the deque mailbox.
     */
    Builder()
        : name_(boost::none_t{})
        , mailboxFactory_([] () { return std::unique_ptr<Mailbox>(new DequeMailbox()); })
        , bond_(nullptr)
        {}

    /**
     * Initializes a builder with the given values.
     */
    Builder(
        const boost::optional<std::string>& name,
        const std::function<std::unique_ptr<Mailbox>()>& mailboxFactory,
        Scheduler* bond)
        : name_(name)
        , mailboxFactory_(mailboxFactory)
        , bond_(bond)
        {}

    /**
     * Copies a builder.
     */
    /// @{
    Builder(const Builder&) = default;
    Builder& operator = (const Builder&) = default;
    /// @}

    /**
     * Moves a builder
     */
    /// @{
    Builder(Builder&&) = default;
    Builder& operator = (Builder&&) = default;
    /// @}

    /**
     * Creates a new builder that is going to build entities
     * bound to the currently running scheduler.
     */
    Builder bound() const {
        return Builder(name(), mailboxFactory(), Scheduler::current());
    }

    /**
     * Creates a new builder that is going to build entities
     * bound to the the given scheduler.
     */
    Builder bound(Scheduler* scheduler) const {
        return Builder(name(), mailboxFactory(), scheduler);
    }

    /**
     * Creates a new builder that is going to build entities
     * not bound to any scheduler.
     */
    Builder unbound() const {
        return Builder(name(), mailboxFactory(), nullptr);
    }

    /**
     * Creates a new builder that is going to build named entities.
     */
    Builder named(const std::string& name) const {
        return Builder(name, mailboxFactory(), bound());
    }

    /**
     * Creates a new builder that is going to build unnamed entities.
     */
    Builder unnamed() const {
        return Builder(boost::none_t{}, mailboxFactory(), bound());
    }

    /**
     * Creates a new builder that is going to build entities with the
     * given mailbox type.
     *
     * @todo Forward parameters to the mailbox.
     */
    template <typename MailboxType>
    Builder withMailbox() const {
        static_assert(boost::is_base_of<Mailbox, MailboxType>::value_type,
            "The given mailbox type must be derived from Maiblox.");
        return Builder(name(), [] () {
            return std::unique_ptr<Mailbox>(new MailboxType);
        }, bound());
    }

    /**
     * The name of the entities built by this builder, if they are
     * named. By default they are unnamed.
     */
    boost::optional<std::string> name() const {
        return name_;
    }

    /**
     * Returns the mailbox factory used to construct the mailbox
     * for the runnable entity.
     */
    std::function<std::unique_ptr<Mailbox>()> mailboxFactory() const {
        return mailboxFactory_;
    }

    /**
     * Scheduler the entities built by this builder are bound to.
     * By default this is nullptr.
     */
    Scheduler* bond() const {
        return bond_;
    }

    /**
     * Creates an identifier from this builder's name. If the name is not
     * set then this returns an newly generated unique id.
     */
    Ident ident() const {
        if (name_.is_initialized()) {
            return name_.get();
        } else {
            return uniqueIdentGenerator.generate();
        }
    }

    /**
     * Runs a new entity constructed using the given arguments and
     * returns its reference.
     *
     * @warning This must be executed on a thread with an attached scheduler.
     */
    template <typename... Args>
    typename EntityTraits::RefType run(Args&&... args) const;

    /**
     * Runs a new entity constructed using the given arguments.
     *
     * @warning This must be executed on a thread with an attached scheduler.
     */
    template <typename... Args>
    void run_(Args&&... args) const;

private:
    const boost::optional<std::string> name_;
    const std::function<std::unique_ptr<Mailbox>()> mailboxFactory_;
    Scheduler* const bond_;
};

} // namespace fiberize

#endif // FIBERIZE_BUILDER_HPP
