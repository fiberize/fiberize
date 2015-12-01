/**
 * Intrusive reference counting.
 *
 * @file referencecounted.cpp
 * @copyright 2015 Paweł Nowak
 */

#include <fiberize/detail/refrencecounted.hpp>

namespace fiberize {
namespace detail {

ReferenceCountedAtomic::ReferenceCountedAtomic() : count(0) {}
ReferenceCountedAtomic::~ReferenceCountedAtomic() {}

void ReferenceCountedAtomic::grab() {
    std::atomic_fetch_add_explicit(&count, 1u, std::memory_order_relaxed);
}

void ReferenceCountedAtomic::drop() {
    if (std::atomic_fetch_sub_explicit(&count, 1u, std::memory_order_release) == 1u) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

void intrusive_ptr_add_ref(ReferenceCountedAtomic* ptr) {
    ptr->grab();
}

void intrusive_ptr_release(ReferenceCountedAtomic* ptr) {
    ptr->drop();
}

ReferenceCounted::ReferenceCounted() : count(0) {}
ReferenceCounted::~ReferenceCounted() {}

void ReferenceCounted::grab() {
    count += 1;
}

void ReferenceCounted::drop() {
    count -= 1;
    if (count == 0) {
        delete this;
    }
}

void intrusive_ptr_add_ref(ReferenceCounted* ptr) {
    ptr->grab();
}

void intrusive_ptr_release(ReferenceCounted* ptr) {
    ptr->drop();
}

} // namespace detail
} // namespace fiberize
