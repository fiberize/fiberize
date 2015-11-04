#ifndef FIBERIZE_DETAIL_FIBERBASE_HPP
#define FIBERIZE_DETAIL_FIBERBASE_HPP

namespace fiberize {
namespace detail {
    
struct ControlBlock;
    
class FiberBase {
public:
    virtual void entryPoint(ControlBlock* controlBlock);
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_FIBERBASE_HPP
