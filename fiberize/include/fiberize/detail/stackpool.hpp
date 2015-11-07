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

class CachedFixedSizeStackPool : public StackPool {
public:
    CachedFixedSizeStackPool();
    
    // StackPool
    virtual ~CachedFixedSizeStackPool();
    virtual boost::context::stack_context allocate();
    virtual void deallocate(boost::context::stack_context stack);
    virtual void delayedDeallocate(boost::context::stack_context stack);
    
private:
    const uint32_t maxSize;
    boost::circular_buffer<boost::context::stack_context> pool;
    boost::context::fixedsize_stack allocator;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_STACKPOOL_HPP
