#ifndef FIBERIZE_DETAIL_RUNNABLE_HPP
#define FIBERIZE_DETAIL_RUNNABLE_HPP

#include <utility>
#include <type_traits>

namespace fiberize {
namespace detail {

/**
 * Type-erased, non-movable and non-copyable, no argument function.
 */
class ErasedRunnable {
public:
    ErasedRunnable() = default;
    ErasedRunnable(const ErasedRunnable&) = delete;
    ErasedRunnable(ErasedRunnable&&) = delete;
    ErasedRunnable& operator = (const ErasedRunnable&) = delete;
    ErasedRunnable& operator = (ErasedRunnable&&) = delete;

    virtual ~ErasedRunnable() {};
    virtual void run() = 0;
};

template <typename Closure>
class Runnable : public ErasedRunnable {
public:
    Runnable() = delete;
    explicit Runnable(Closure closure) : closure(std::move(closure)) {}

    void run() override {
        closure();
    }

private:
    Closure closure;
};

template <typename Closure>
std::unique_ptr<Runnable<Closure>> makeRunnable(Closure closure) {
    return std::unique_ptr<Runnable<Closure>>(new Runnable<Closure>(std::move(closure)));
}

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_RUNNABLE_HPP
