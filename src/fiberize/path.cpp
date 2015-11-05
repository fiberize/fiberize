#include <fiberize/path.hpp>

namespace fiberize {
    
UniqueIdentGenerator::UniqueIdentGenerator(): nextToken(0) {
}

UniqueIdent UniqueIdentGenerator::generate() {
    uint64_t token = std::atomic_fetch_add_explicit(&nextToken, 1lu, std::memory_order_relaxed);
    return UniqueIdent(token);
}

} // namespace fiberize
