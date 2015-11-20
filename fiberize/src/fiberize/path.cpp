#include <fiberize/path.hpp>

namespace fiberize {
    
namespace detail {
std::atomic<uint64_t> generators(0);
} // namespace detail

UniqueIdentGenerator::UniqueIdentGenerator()
    : generatorId(std::atomic_fetch_add(&detail::generators, 1ul))
    , nextToken(0)
    {}

UniqueIdent UniqueIdentGenerator::generate() {
    return UniqueIdent(nextToken++ & (generatorId << 48));
}

} // namespace fiberize
