#ifndef FIBERIZE_DETAIL_STACKPOOL_HPP
#define FIBERIZE_DETAIL_STACKPOOL_HPP

#include <boost/context/all.hpp>
#include <boost/circular_buffer.hpp>

namespace fiberize {
namespace detail {

/**
 * Manages stack allocation and reuse.
 */
class StackPool {
public:
    virtual ~StackPool() {};
    virtual boost::context::stack_context allocate() = 0;
    virtual void deallocate(boost::context::stack_context stack) = 0;
    virtual void delayedDeallocate(boost::context::stack_context stack) = 0;
};

#ifdef FIBERIZE_SEGMENTED_STACKS

class SegmentedStackPool : public StackPool {
public:
    SegmentedStackPool();

    // StackPool
    ~SegmentedStackPool() override;
    boost::context::stack_context allocate() override;
    void deallocate(boost::context::stack_context stack) override;
    void delayedDeallocate(boost::context::stack_context stack) override;

private:
    size_t inUse;
    std::vector<boost::context::stack_context> pool;
    boost::context::stack_context delayed;
    boost::context::segmented_stack allocator;
};

typedef SegmentedStackPool DefaultStackPool;

#else // if !FIBERIZE_SEGMENTED_STACKS

class CachedFixedSizeStackPool : public StackPool {
public:
    CachedFixedSizeStackPool();

    // StackPool
    ~CachedFixedSizeStackPool() override;
    boost::context::stack_context allocate() override;
    void deallocate(boost::context::stack_context stack) override;
    void delayedDeallocate(boost::context::stack_context stack) override;

private:
    size_t inUse;
    std::vector<boost::context::stack_context> pool;
    boost::context::stack_context delayed;
    boost::context::protected_fixedsize_stack allocator;
};

typedef CachedFixedSizeStackPool DefaultStackPool;

#endif // !FIBERIZE_SEGMENTED_STACKS

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_STACKPOOL_HPP
