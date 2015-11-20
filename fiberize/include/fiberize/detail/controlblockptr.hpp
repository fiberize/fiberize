#ifndef FIBERIZE_DETAIL_CONTROLBLOCKPTR_HPP
#define FIBERIZE_DETAIL_CONTROLBLOCKPTR_HPP

#include <atomic>

namespace fiberize {
namespace detail {

class ControlBlock;

class ControlBlockPtr {
public:
    inline ControlBlockPtr() : ptr(nullptr) {}
    explicit inline ControlBlockPtr(ControlBlock* ptr) { assign(ptr); }
    inline ControlBlockPtr(const ControlBlockPtr& other) { assign(other.get()); }
    inline ControlBlockPtr(ControlBlockPtr&& other) : ptr(other.ptr) { other.ptr = nullptr; }
    inline ~ControlBlockPtr() { release(); }

    inline ControlBlockPtr& operator = (const ControlBlockPtr& other) {
        release();
        assign(other.ptr);
        return *this;
    }

    inline ControlBlockPtr& operator = (ControlBlockPtr&& other) {
        release();
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    inline bool operator == (const ControlBlockPtr& other) {
        return ptr == other.ptr;
    }

    inline bool operator != (const ControlBlockPtr& other) {
        return ptr != other.ptr;
    }

    inline ControlBlock* get() const {
        return ptr;
    }

    inline ControlBlock* operator -> () const {
        return ptr;
    }

    inline ControlBlock& operator * () const {
        return *ptr;
    }

private:
    void release();
    void assign(ControlBlock* newPtr);

    ControlBlock* ptr;
};

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_CONTROLBLOCKPTR_HPP
