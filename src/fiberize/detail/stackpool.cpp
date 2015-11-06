#include <fiberize/detail/stackpool.hpp>

namespace fiberize {
namespace detail {

// TODO: get rid of magic numbers and allow configuration
CachedFixedSizeStackPool::CachedFixedSizeStackPool()
    : maxSize(100), allocator(), pool(maxSize) {}

CachedFixedSizeStackPool::~CachedFixedSizeStackPool() {
    for (auto stack : pool)
        allocator.deallocate(stack);
}

boost::context::stack_context CachedFixedSizeStackPool::allocate() {
    if (pool.empty()) {
        return allocator.allocate();
    } else {
        auto stack = pool.back();
        pool.pop_back();
        return stack;
    }
}

void CachedFixedSizeStackPool::deallocate(boost::context::stack_context stack) {
    if (pool.size() < maxSize) {
        pool.push_back(stack);
    } else {
        allocator.deallocate(stack);
    }
}

void CachedFixedSizeStackPool::delayedDeallocate(boost::context::stack_context stack) {
    if (pool.size() == maxSize) {
        allocator.deallocate(pool.front());
        pool.pop_front();
    }
    
    pool.push_back(stack);
}

} // namespace detail
} // namespace fiberize
