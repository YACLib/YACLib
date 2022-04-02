#pragma once

#include <yaclib/algo/detail/when_any_impl.hpp>
#include <yaclib/algo/detail/when_impl.hpp>
#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>
#include <iterator>
#include <utility>

namespace yaclib {

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , size the range of futures to combine
 * \return Future<T>
 */
template <WhenPolicy P = WhenPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
auto WhenAny(It begin, std::size_t size) {
  static_assert(is_future_v<T>, "WhenAny function Iterator must be point to some Future");
  using V = future_value_t<T>;
  using E = future_error_t<T>;
  if (size == 0) {
    return Future<V, E>{};
  }
  if (size == 1) {
    return std::move(*begin);
  }
  auto [future, combinator] = detail::AnyCombinator<V, E, P>::Make(size);
  detail::WhenImpl(combinator, begin, size);
  return std::move(future);
}

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , end the range of futures to combine
 * \return Future<T>
 */
template <WhenPolicy P = WhenPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
auto WhenAny(It begin, It end) {
  static_assert(is_future_v<T>, "WhenAny function Iterator must be point to some Future");
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAny<P>(begin, end - begin);
}

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam V type of value all passed futures
 * \tparam E type of error all passed futures
 * \param head , tail one or more futures to combine
 * \return Future<T>
 */
template <WhenPolicy P = WhenPolicy::LastFail, typename V, typename E, typename... Vs, typename Es>
Future<V, E> WhenAny(Future<V, E>&& head, Future<Vs, Es>&&... tail) {
  constexpr std::size_t kSize = 1 + sizeof...(Vs);
  static_assert(kSize >= 2, "WhenAny wants at least two futures");
  auto [future, combinator] = detail::AnyCombinator<V, E, P>::Make(kSize);
  detail::WhenImpl(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib
