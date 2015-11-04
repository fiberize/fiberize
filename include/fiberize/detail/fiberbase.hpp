#ifndef FIBERIZE_DETAIL_FIBERBASE_HPP
#define FIBERIZE_DETAIL_FIBERBASE_HPP

namespace fiberize {
namespace detail {
    
struct ControlBlock;
    
class FiberBase {
public:
    virtual ~FiberBase() {};
    virtual void entryPoint() = 0;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_FIBERBASE_HPP
