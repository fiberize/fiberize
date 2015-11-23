#include <fiberize/detail/stackpool.hpp>

namespace fiberize {
namespace detail {

constexpr size_t minCached = 32;

#ifdef FIBERIZE_SEGMENTED_STACKS

SegmentedStackPool::SegmentedStackPool()
    : inUse(0), pool(), allocator(boost::context::segmented_stack::traits_type::minimum_size()) {}

SegmentedStackPool::~SegmentedStackPool() {
    for (auto stack : pool)
        allocator.deallocate(stack);
    if (delayed.sp != nullptr)
        allocator.deallocate(delayed);
}

boost::context::stack_context SegmentedStackPool::allocate() {
    inUse += 1;
    if (pool.empty()) {
        return allocator.allocate();
    } else {
        auto stack = pool.back();
        pool.pop_back();
        return stack;
    }
}

void SegmentedStackPool::deallocate(boost::context::stack_context stack) {
    inUse -= 1;

    if (pool.size() < minCached + inUse / 2) {
        pool.push_back(stack);
    } else {
        allocator.deallocate(stack);
    }
}

void SegmentedStackPool::delayedDeallocate(boost::context::stack_context stack) {
    if (delayed.sp != nullptr) {
        deallocate(delayed);
    }

    delayed = stack;
}

#else // if !FIBERIZE_SEGMENTED_STACKS

// TODO: get rid of magic numbers and allow configuration
CachedFixedSizeStackPool::CachedFixedSizeStackPool()
    : inUse(0), pool(), allocator() {}

CachedFixedSizeStackPool::~CachedFixedSizeStackPool() {
    for (auto stack : pool)
        allocator.deallocate(stack);
    if (delayed.sp != nullptr)
        allocator.deallocate(delayed);
}

boost::context::stack_context CachedFixedSizeStackPool::allocate() {
    inUse += 1;
    if (pool.empty()) {
        return allocator.allocate();
    } else {
        auto stack = pool.back();
        pool.pop_back();
        return stack;
    }
}

void CachedFixedSizeStackPool::deallocate(boost::context::stack_context stack) {
    inUse -= 1;

    if (pool.size() < minCached + inUse / 2) {
        pool.push_back(stack);
    } else {
        allocator.deallocate(stack);
    }
}

void CachedFixedSizeStackPool::delayedDeallocate(boost::context::stack_context stack) {
    if (delayed.sp != nullptr) {
        deallocate(delayed);
    }

    delayed = stack;
}

#endif // !FIBERIZE_SEGMENTED_STACKS

} // namespace detail
} // namespace fiberize
