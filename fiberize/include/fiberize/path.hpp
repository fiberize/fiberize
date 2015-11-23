#ifndef FIBERIZE_PATH_HPP
#define FIBERIZE_PATH_HPP

#include <atomic>

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>

namespace fiberize {

/**
 * An Ident identifying a resource by a human readable name.
 */
struct NamedIdent {
public:
    inline NamedIdent(const std::string& name): name_(name) {};
    inline NamedIdent(std::string&& name): name_(std::move(name)) {};
    NamedIdent(const NamedIdent&) = default;
    NamedIdent(NamedIdent&&) = default;
    
    NamedIdent& operator = (const NamedIdent&) = default;
    NamedIdent& operator = (NamedIdent&&) = default;
    
    inline bool operator == (const NamedIdent& other) const { return name_ == other.name_; };
    inline bool operator != (const NamedIdent& other) const { return name_ != other.name_; };
    
    inline std::string name() const {
        return name_;
    }
    
    inline std::size_t hash() const {
        return boost::hash_value(name_);
    }
    
private:
    std::string name_;
};

inline std::size_t hash_value(const NamedIdent& ident) {
    return ident.hash();
}

/**
 * An Ident generated when you create a new unique resource.
 */
struct UniqueIdent {
public:
    inline UniqueIdent(uint64_t token): token_(token) {};
    UniqueIdent(const UniqueIdent&) = default;
    UniqueIdent(UniqueIdent&&) = default;
    
    UniqueIdent& operator = (const UniqueIdent&) = default;
    UniqueIdent& operator = (UniqueIdent&&) = default;
    
    inline bool operator == (const UniqueIdent& other) const { return token_ == other.token_; };
    inline bool operator != (const UniqueIdent& other) const { return token_ != other.token_; };
    
    inline uint64_t token() const {
        return token_;
    }
    
    inline std::size_t hash() const {
        return boost::hash_value(token_);
    }
    
private:
    uint64_t token_;
};

inline std::size_t hash_value(const UniqueIdent& ident) {
    return ident.hash();
}

/**
 * A part of Path identifying a resource on a specific system.
 */
typedef boost::variant<NamedIdent, UniqueIdent> Ident;

/**
 * The /dev/null path, representing a resource that doesn't exist.
 * @see Path
 */
struct DevNullPath {
public:
    inline DevNullPath() {};
    DevNullPath(const DevNullPath&) = default;
    DevNullPath(DevNullPath&&) = default;
    
    DevNullPath& operator = (const DevNullPath&) = default;
    DevNullPath& operator = (DevNullPath&&) = default;
    
    inline bool operator == (const DevNullPath&) const { return true; };
    inline bool operator != (const DevNullPath&) const { return false; };
    
    inline std::size_t hash() const {
        return 0;
    }
};

inline std::size_t hash_value(const DevNullPath& path) {
    return path.hash();
}

/**
 * A prefixed path, representing a resource on a specific system.
 * @see Path
 */
struct PrefixedPath {
public:
    inline PrefixedPath(const boost::uuids::uuid& prefix, const Ident& ident): prefix_(prefix), ident_(ident) {};
    PrefixedPath(const PrefixedPath&) = default;
    PrefixedPath(PrefixedPath&&) = default;
    
    PrefixedPath& operator = (const PrefixedPath&) = default;
    PrefixedPath& operator = (PrefixedPath&&) = default;
    
    inline bool operator == (const PrefixedPath& other) const { return prefix_ == other.prefix_ && ident_ == other.ident_; };
    inline bool operator != (const PrefixedPath& other) const { return prefix_ != other.prefix_ || ident_ != other.ident_; };
    
    inline std::size_t hash() const {
        boost::hash<boost::uuids::uuid> uuidHasher;
        size_t seed = uuidHasher(prefix_);
        boost::hash_combine(seed, boost::hash_value(ident_));
        return seed;
    }
    
private:
    boost::uuids::uuid prefix_;
    Ident ident_;
};

inline std::size_t hash_value(const PrefixedPath& path) {
    return path.hash();
}

/**
 * A global path, refering to a resource that could be located on any fibery system.
 * @see Path
 */
struct GlobalPath {
public:
    inline GlobalPath(const Ident& ident): ident_(ident) {};
    GlobalPath(const GlobalPath&) = default;
    GlobalPath(GlobalPath&&) = default;
    
    GlobalPath& operator = (const GlobalPath&) = default;
    GlobalPath& operator = (GlobalPath&&) = default;
    
    inline bool operator == (const GlobalPath& other) const { return ident_ == other.ident_; };
    inline bool operator != (const GlobalPath& other) const { return ident_ != other.ident_; };
    
    inline std::size_t hash() const {
        return boost::hash_value(ident_);
    }

private:
    Ident ident_;
};

inline std::size_t hash_value(const GlobalPath& path) {
    return path.hash();
}

/**
 * A path to a resource. Resources include fibers and events. There are three types of paths:
 *  - the /dev/null path, which refers to a nonexisting resource,
 *  - a prefixed path, refering to a resource on a particular fiber system,
 *  - a global path, refering to a resource that could be located on any fibery system.
 * 
 * Global paths should be used for events shared between systems and for singleton fibers.
 */
typedef boost::variant<DevNullPath, PrefixedPath, GlobalPath> Path;

/**
 * Unique ident generator.
 *
 * It's not thread safe, as it was designed to be used as a thread local variable.
 * Values generated by multiple instances of this class will still be unique.
 * You can only create 2^16 generator instance.
 */
class UniqueIdentGenerator {
public:
    UniqueIdentGenerator();
    
    /**
     * Generated an unique identifier.
     */
    UniqueIdent generate();
    
private:
    uint64_t generatorId;
    uint64_t nextToken;
};

/**
 * Generator used to name new fibers and events.
 */
extern thread_local UniqueIdentGenerator uniqueIdentGenerator;

} // nemespace fiberize

#endif // FIBERIZE_PATH_HPP
