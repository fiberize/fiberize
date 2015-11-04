#ifndef FIBERIZE_DETAIL_FIBERBASE_HPP
#define FIBERIZE_DETAIL_FIBERBASE_HPP

#include <fiberize/buffer.hpp>

namespace fiberize {
namespace detail {
    
struct ControlBlock;
    
class FiberBase {
public:
    virtual ~FiberBase() {};
    
    /**
     * Run the fiber and store the result in a buffer.
     */
    virtual Buffer runStored() = 0;
};
    
} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_FIBERBASE_HPP
