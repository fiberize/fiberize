#include <fiberize/path.hpp>

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

} // namespace fiberize
