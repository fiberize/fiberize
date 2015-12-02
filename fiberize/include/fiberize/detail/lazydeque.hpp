/**
 * Deque with lazy intialization.
 *
 * @file dequequeue.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_LAZYDEQUE_HPP
#define FIBERIZE_DETAIL_LAZYDEQUE_HPP

#include <deque>
#include <iostream>

#include <boost/optional.hpp>

namespace fiberize {
namespace detail {

template <typename A>
class LazyDeque {
public:
    struct WrappedIterator {
        boost::optional<typename std::deque<A>::iterator> iterator;

        bool operator == (const WrappedIterator& other) const { return iterator == other.iterator; }
        bool operator != (const WrappedIterator& other) const { return iterator != other.iterator; }

        A& operator * () { return iterator.get().operator * (); }
        A* operator -> () { return iterator.get().operator -> (); }

        WrappedIterator& operator ++ () { ++iterator.get(); return *this; }
        WrappedIterator operator ++ (int) { return WrappedIterator{iterator.get()++}; }
    };

    using iterator = WrappedIterator;

    LazyDeque() = default;
    LazyDeque(const LazyDeque&) = default;
    LazyDeque(LazyDeque&&) = default;

    iterator begin() {
        if (queue) {
            return WrappedIterator{queue.get().begin()};
        } else {
            return WrappedIterator{};
        }
    }

    iterator end() {
        if (queue) {
            return WrappedIterator{queue.get().end()};
        } else {
            return WrappedIterator{};
        }
    }

    void push_front(const A& value) {
        if (!queue)
            queue.emplace();
        queue.get().push_front(value);
    }

    void push_back(const A& value) {
        if (!queue)
            queue.emplace();
        queue.get().push_back(value);
    }

    void pop_front() {
        queue.get().pop_front();
    }

    void pop_back() {
        queue.get().pop_back();
    }

    A& front() {
        return queue.get().front();
    }

    A& back() {
        return queue.get().back();
    }

    bool empty() {
        return !queue || queue.get().empty();
    }

    A& operator [] (size_t index) {
        return queue.get()[index];
    }

private:
    boost::optional<std::deque<A>> queue;
};

template <typename A>
void swap(typename LazyDeque<A>::iterator& x, typename LazyDeque<A>::iterator& y) {
    swap(x.iterator, y.iterator);
}

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_LAZYDEQUE_HPP
