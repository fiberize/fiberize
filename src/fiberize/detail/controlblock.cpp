#include <fiberize/detail/controlblock.hpp>

namespace fiberize {
namespace detail {    

void ControlBlock::grab() {
    std::atomic_fetch_add_explicit(&refCount, 1lu, std::memory_order_relaxed);
}

bool ControlBlock::drop() {
    if (std::atomic_fetch_sub_explicit(&refCount, 1lu, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
        return true;
    }
    return false;
}
    
} // namespace detail
} // namespace fiberize
