#include <fiberize/path.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>

namespace fiberize {

Path devNullPath(DevNullPath{});

namespace detail {
std::atomic<uint64_t> generators(0);
} // namespace detail

UniqueIdentGenerator::UniqueIdentGenerator()
    : generatorId(std::atomic_fetch_add(&detail::generators, 1ul))
    , nextToken(0)
    {}

UniqueIdent UniqueIdentGenerator::generate() {
    return UniqueIdent((nextToken++) | (generatorId << 48));
}

thread_local UniqueIdentGenerator uniqueIdentGenerator;

struct IdentFormatter {
    using result_type = std::string;

    std::string operator () (const NamedIdent& ident) {
        return ":" + ident.name();
    }

    std::string operator () (const UniqueIdent& ident) {
        return "#" + std::to_string(ident.token());
    }
};

std::string toString(const Ident& ident) {
    IdentFormatter formatter;
    return ident.apply_visitor(formatter);
}

struct PathFormatter {
    using result_type = std::string;

    std::string operator () (const DevNullPath&) {
        return "";
    }

    std::string operator () (const GlobalPath& path) {
        return toString(path.ident());
    }

    std::string operator () (const PrefixedPath& path) {
        return to_string(path.prefix()) + toString(path.ident());
    }
};

std::string toString(const Path& path) {
    PathFormatter formatter;
    return path.apply_visitor(formatter);
}

} // namespace fiberize
