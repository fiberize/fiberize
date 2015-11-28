/**
 * Intrusive reference counting.
 *
 * @file referencecounted.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_REFERENCECOUNTED_HPP
#define FIBERIZE_DETAIL_REFERENCECOUNTED_HPP

#include <atomic>

namespace fiberize {
namespace detail {

class ReferenceCountedAtomic {
public:
    ReferenceCountedAtomic();
    ReferenceCountedAtomic(const ReferenceCountedAtomic&) = delete;
    ReferenceCountedAtomic(ReferenceCountedAtomic&&) = delete;
    virtual ~ReferenceCountedAtomic();

    /**
     * Increases the reference count by 1.
     * @note Thread-safe.
     */
    void grab();

    /**
     * Decreases the reference count by 1. When the count goes down to 0 the object is destroyed.
     * @note Thread-safe.
     */
    void drop();

private:
    std::atomic<uint32_t> count;
};

void intrusive_ptr_add_ref(ReferenceCountedAtomic* ptr);
void intrusive_ptr_release(ReferenceCountedAtomic* ptr);

class ReferenceCounted {
public:
    ReferenceCounted();
    ReferenceCounted(const ReferenceCounted&) = delete;
    ReferenceCounted(ReferenceCounted&&) = delete;
    virtual ~ReferenceCounted();

    /**
     * Increases the reference count by 1.
     * @warning Not thread-safe.
     */
    void grab();

    /**
     * Decreases the reference count by 1. When the count goes down to 0 the object is destroyed.
     * @warning Not thread-safe.
     */
    void drop();

private:
    uint32_t count;
};

void intrusive_ptr_add_ref(ReferenceCounted* ptr);
void intrusive_ptr_release(ReferenceCounted* ptr);

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_REFERENCECOUNTED_HPP
