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
 * entity with the run (or run_) function. The builder itself is
 * immutable and modyfing paremeters returns a new builder.
 */
template <typename EntityTraits, typename Entity, typename MailboxType>
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
        Entity entity,
        MailboxType mailbox,
        Scheduler* bond)
        : name_(std::move(name))
        , entity_(std::move(entity))
        , mailbox_(std::move(mailbox))
        , bond_(bond)
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
     * Returns the entity.
     */
    Entity& entity() const {
        return entity_;
    }

    /**
     * Returns the mailbox.
     */
    MailboxType& mailbox() const {
        return mailbox_;
    }

    /**
     * Scheduler the entities built by this builder are bound to.
     */
    Scheduler*& bond() const {
        return bond_;
    }

    ///@}

    /**
     * @name Modyfing the builder.
     *
     * Methods in this section are used to modify the parameters of the runnable entity.
     * Instead of modyfing the builder directly, they return a new one, with old variables
     * moved (instead of copied) to the new one. This invalidates the old builder. If you
     * want to use a builder more then once, you have to copy() it.
     */
    ///@{

    /**
     * Creates a new builder that is going to build an entity bound to the currently running scheduler.
     * @warning This invalidates the current builder.
     */
    Builder bound() {
        return Builder(std::move(name_), std::move(entity_), std::move(mailbox_), Scheduler::current());
    }

    /**
     * Creates a new builder that is going to build an entity bound to the the given scheduler.
     * @warning This invalidates the current builder.
     */
    Builder bound(Scheduler* scheduler) {
        return Builder(std::move(name_), std::move(entity_), std::move(mailbox_), scheduler);
    }

    /**
     * Creates a new builder that is going to build an entity not bound to any scheduler.
     * @warning This invalidates the current builder.
     */
    Builder unbound() const {
        return Builder(std::move(name_), std::move(entity_), std::move(mailbox_), nullptr);
    }

    /**
     * Creates a new builder that is going to build a named entity.
     * @warning This invalidates the current builder.
     */
    Builder named(std::string name) const {
        return Builder(std::move(name), std::move(entity_), std::move(mailbox_), bond_);
    }

    /**
     * Creates a new builder that is going to build an unnamed entity.
     * @warning This invalidates the current builder.
     */
    Builder unnamed() const {
        return Builder(boost::none_t{}, std::move(entity_), std::move(mailbox_), bond_);
    }

    /**
     * Creates a new builder that is going to build an entity with the given mailbox.
     * @warning This invalidates the current builder.
     */
    template <typename NewMailboxType>
    Builder<EntityTraits, Entity, NewMailboxType> withMailbox(std::unique_ptr<NewMailboxType> newMailbox) const {
        static_assert(boost::is_base_of<Mailbox, MailboxType>::value_type,
            "The given mailbox type must be derived from Maiblox.");
        mailbox_.reset();
        return Builder<EntityTraits, Entity, NewMailboxType>(
            std::move(name_), std::move(entity_), std::move(newMailbox), bond_
        );
    }

    ///@}

    /**
     * @name Running
     */
    ///@{

    /**
     * Runs the entity using the given arguments and returns a reference.
     * @warning This invalidates the builder.
     * @warning This must be executed on a thread with an attached scheduler.
     */
    template <typename... Args>
    typename EntityTraits::template For<Entity>::template WithArgs<Args...>::RefType
    run(Args&&... args);

    /**
     * Runs the entity using the given arguments.
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
    Entity entity_;
    MailboxType mailbox_;
    Scheduler* const bond_;
};

} // namespace fiberize

#endif // FIBERIZE_BUILDER_HPP
