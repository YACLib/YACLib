#pragma once

#include <yaclib/algo/detail/when_any_impl.hpp>
#include <yaclib/algo/when_policy.hpp>
#include <yaclib/async/future.hpp>
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
  return detail::WhenAnyImpl<P, future_value_t<T>, future_error_t<T>>(begin, std::size_t{0}, size);
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
  return detail::WhenAnyImpl<P, future_value_t<T>, future_error_t<T>>(begin, begin, end);
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
  detail::WhenAnyImpl<P>(combinator, std::move(head), std::move(tail)...);
  return std::move(future);
}

}  // namespace yaclib
