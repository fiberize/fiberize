#ifndef FIBERIZE_PATH_HPP
#define FIBERIZE_PATH_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>

namespace fiberize {

/**
 * An Ident identifying a resource by a human readable name.
 */
struct NamedIdent {
public:
    NamedIdent(std::string&& name): name_(name) {};
    NamedIdent(const NamedIdent&) = default;
    NamedIdent(NamedIdent&&) = default;
    
    NamedIdent& operator = (const NamedIdent&) = default;
    NamedIdent& operator = (NamedIdent&&) = default;
    
    bool operator == (const NamedIdent& other) const { return name_ == other.name_; };
    bool operator != (const NamedIdent& other) const { return name_ != other.name_; };
    
private:
    std::string name_;
};

/**
 * An Ident generated when you create a new unique resource.
 */
struct UniqueIdent {
public:
    UniqueIdent(uint64_t token): token_(token) {};
    UniqueIdent(const UniqueIdent&) = default;
    UniqueIdent(UniqueIdent&&) = default;
    
    UniqueIdent& operator = (const UniqueIdent&) = default;
    UniqueIdent& operator = (UniqueIdent&&) = default;
    
    bool operator == (const UniqueIdent& other) const { return token_ == other.token_; };
    bool operator != (const UniqueIdent& other) const { return token_ != other.token_; };
    
private:
    uint64_t token_;
};

/**
 * A part of Path identifying a resource on a specific system.
 */
typedef boost::variant<NamedIdent, UniqueIdent> Ident;

/**
 * The /dev/null path, representing a resource that doesn't exist.
 * @see Path
 */
struct DevNullPath {};

/**
 * A prefixed path, representing a resource on a specific system.
 * @see Path
 */
struct PrefixedPath {
public:
    PrefixedPath(const boost::uuids::uuid& prefix, const Ident& ident);
    PrefixedPath(const PrefixedPath&) = default;
    PrefixedPath(PrefixedPath&&) = default;
    
    PrefixedPath& operator = (const PrefixedPath&) = default;
    PrefixedPath& operator = (PrefixedPath&&) = default;
    
    bool operator == (const PrefixedPath& other) const { return prefix_ == other.prefix_ && ident_ == other.ident_; };
    bool operator != (const PrefixedPath& other) const { return prefix_ != other.prefix_ || ident_ != other.ident_; };
    
private:
    boost::uuids::uuid prefix_;
    Ident ident_;
};

/**
 * A global path, refering to a resource that could be located on any fibery system.
 * @see Path
 */
struct GlobalPath {
public:
    GlobalPath(const Ident& ident);
    GlobalPath(const GlobalPath&) = default;
    GlobalPath(GlobalPath&&) = default;
    
    GlobalPath& operator = (const GlobalPath&) = default;
    GlobalPath& operator = (GlobalPath&&) = default;
    
    bool operator == (const GlobalPath& other) const { return ident_ == other.ident_; };
    bool operator != (const GlobalPath& other) const { return ident_ != other.ident_; };
    
private:
    Ident ident_;
};

/**
 * A path to a resource. Resources include fibers and events. There are three types of paths:
 *  - the /dev/null path, which refers to a nonexisting resource,
 *  - a prefixed path, refering to a resource on a particular fiber system,
 *  - a global path, refering to a resource that could be located on any fibery system.
 * 
 * Global paths should be used for events shared between systems and for singleton fibers.
 */
typedef boost::variant<DevNullPath, PrefixedPath, GlobalPath> Path;

} // nemespace fiberize

#endif // FIBERIZE_PATH_HPP
