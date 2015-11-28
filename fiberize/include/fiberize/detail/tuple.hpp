/**
 * Tuple utilities.
 *
 * @file tuple.hpp
 * @copyright 2015 Paweł Nowak
 */
#ifndef FIBERIZE_DETAIL_TUPLE_HPP
#define FIBERIZE_DETAIL_TUPLE_HPP

#include <tuple>
#include <utility>

namespace fiberize {
namespace detail {

template <typename F, typename Tuple, size_t... I>
auto apply_impl(F&& f, Tuple&& t, index_sequence<I...>) {
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}

/**
 * Applies a tuple to a function.
 *
 * From C++ standard proposal [N3915](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3915.pdf).
 */
template <typename F, typename Tuple>
auto apply(F&& f, Tuple&& t) {
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

} // namespace detail
} // namespace fiberize

#endif // FIBERIZE_DETAIL_TUPLE_HPP
