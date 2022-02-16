#pragma once

#include <yaclib/algo/detail/when_all_impl.hpp>
#include <yaclib/algo/detail/when_impl.hpp>
#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/util/type_traits.hpp>

#include <cstddef>
#include <iterator>
#include <utility>

namespace yaclib {

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , size the range of futures to combine
 * \return Future<std::vector<T>>
 */
template <WhenPolicy P = WhenPolicy::FirstFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
auto WhenAll(It begin, std::size_t size) {
  static_assert(P == WhenPolicy::FirstFail, "TODO(Ri7ay) Add other policy for WhenAll");
  static_assert(is_future_v<T>, "WhenAll function Iterator must be point to some Future");
  auto [future, combinator] = detail::AllCombinator<future_value_t<T>, future_error_t<T>>::Make(size);
  detail::WhenImpl(combinator, begin, size);
  return std::move(future);
}

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \param begin , end the range of futures to combine
 * \return Future<std::vector<T>>
 */
template <WhenPolicy P = WhenPolicy::FirstFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
auto WhenAll(It begin, It end) {
  static_assert(P == WhenPolicy::FirstFail, "TODO(Ri7ay) Add other policy for WhenAll");
  static_assert(is_future_v<T>, "WhenAll function Iterator must be point to some Future");
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAll(begin, distance(begin, end))
  return WhenAll(begin, end - begin);
}

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam V type of value all passed futures
 * \tparam E type of error all passed futures
 * \param head , tail one or more futures to combine
 * \return Future<std::array<T>>
 */
template <WhenPolicy P = WhenPolicy::FirstFail, typename V, typename E, typename... Vs, typename... Es>
auto WhenAll(Future<V, E>&& head, Future<Vs, Es>&&... tail) {
  static_assert(P == WhenPolicy::FirstFail, "TODO(Ri7ay) Add other policy for WhenAll");
  constexpr std::size_t kSize = 1 + sizeof...(Vs);
  static_assert(kSize >= 2, "WhenAll wants at least two futures");
  auto [future, combinator] = detail::AllCombinator<V, E, kSize>::Make(kSize);
  detail::WhenImpl(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib
