#ifndef FIBERIZE_DETAIL_FIBERBASE_HPP
#define FIBERIZE_DETAIL_FIBERBASE_HPP

namespace fiberize {
namespace detail {

struct ControlBlock;

struct FiberBase {
    virtual ~FiberBase() {};
    
    /**
     * Run the fiber and take care of return value and unhandled exceptions.
     */
    virtual void _execute(detail::ControlBlock* controlBlock) = 0;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_FIBERBASE_HPP
