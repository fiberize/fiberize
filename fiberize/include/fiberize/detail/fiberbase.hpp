#ifndef FIBERIZE_DETAIL_FIBERBASE_HPP
#define FIBERIZE_DETAIL_FIBERBASE_HPP

#include <memory>

namespace fiberize {
namespace detail {

struct ControlBlock;

struct FiberBase {
    virtual ~FiberBase() {};
    
    /**
     * Run the fiber and take care of return value and unhandled exceptions.
     */
    virtual void _execute(detail::ControlBlockPtr controlBlock) = 0;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_FIBERBASE_HPP
