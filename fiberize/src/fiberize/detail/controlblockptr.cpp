#include <fiberize/detail/controlblockptr.hpp>
#include <fiberize/detail/controlblock.hpp>

namespace fiberize {
namespace detail {

void ControlBlockPtr::release() {
    if (ptr != nullptr) {
        ptr->drop();
        ptr = nullptr;
    }
}

void ControlBlockPtr::assign(ControlBlock* newPtr) {
    ptr = newPtr;
    if (ptr != nullptr) {
        ptr->grab();
    }
}

} // namespace detail
} // namespace fiberize
