#ifndef FIBERIZE_HANDLER_HPP
#define FIBERIZE_HANDLER_HPP

#include <type_traits>
#include <cinttypes>
#include <utility>
#include <functional>

namespace fiberize {

class HandlerRef;
    
namespace detail {
    
/**
 * A closure bound to an event.
 */
struct Handler {
public:
    inline Handler(): refCount(0) {};
    virtual ~Handler() {};

    virtual void execute(const void* data) = 0;
    
    inline void grab() {
        ++refCount;
    }
    
    inline void drop() {
        if (--refCount == 0) {
            release();
        }
    }
    
    inline bool isDestroyed() {
        return refCount == 0;
    }
    
protected:
    virtual void release() = 0;
    
    /**
     * Number of handler references referencing this handler.
     */
    uint64_t refCount;
};

template <typename A>
class TypedHandler : public Handler {
public:    
    template <typename... Args>
    explicit TypedHandler(Args&&... args) : handler(std::forward<Args>(args...)...) {}

    virtual void execute(const void* data) {
        handler(*reinterpret_cast<const A*>(data));
    }
    
protected:
    virtual void release() {
        handler = std::function<void (const A&)>();
    }
    
    std::function<void (const A&)> handler;
};

template <>
class TypedHandler<void> : public Handler {
public:
    template <typename... Args>
    explicit TypedHandler(Args&&... args) : handler(std::forward<Args>(args...)...) {}

    virtual void execute(const void*) {
        handler();
    }

protected:
    virtual void release() {
        handler = std::function<void ()>();
    }

    std::function<void ()> handler;
};

} // namespace detail

class HandlerRef {
public:
    HandlerRef() : handler(nullptr) {}
    
    explicit HandlerRef(detail::Handler* handler): handler(handler) {
        if (handler != nullptr)
            handler->grab();
    }
    
    HandlerRef(const HandlerRef& ref): handler(ref.handler) {
        if (handler != nullptr)
            handler->grab();
    }
    
    HandlerRef(HandlerRef&& ref): handler(ref.handler) {
        ref.handler = nullptr;
    }
    
    ~HandlerRef() {
        release();
    }
    
    HandlerRef& operator = (const HandlerRef& ref) {
        if (handler != nullptr)
            handler->drop();
        handler = ref.handler;
        if (handler != nullptr)
            handler->grab();
        return *this;
    }
    
    HandlerRef& operator = (HandlerRef&& ref) {
        if (handler != nullptr)
            handler->drop();
        handler = ref.handler;
        ref.handler = nullptr;
        return *this;
    }
    
    void release() {
        if (handler != nullptr) {
            handler->drop();
            handler = nullptr;
        }
    }
    
private:
    detail::Handler* handler;
};    
    
} // namespace fiberize

#endif // FIBERIZE_HANDLER_HPP

