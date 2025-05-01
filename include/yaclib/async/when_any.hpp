#pragma once

#include <yaclib/async/detail/when_any_impl.hpp>
#include <yaclib/async/detail/when_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
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
 * \param begin , count the range of futures to combine
 * \return Future<T>
 */
template <FailPolicy P = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
auto WhenAny(It begin, std::size_t count) {
  static_assert(is_future_base_v<T>, "WhenAny function Iterator must be point to some Future");
  using V = future_base_value_t<T>;
  using E = future_base_error_t<T>;
  YACLIB_WARN(count < 2, "Don't use combinators for zero or one futures");
  if (count == 0) {
    return Future<V, E>{};
  }
  if (count == 1) {
    return Future<V, E>{std::exchange(begin->GetCore(), nullptr)};
  }
  auto [future_core, combinator] = detail::AnyCombinator<V, E, P>::Make(count);
  detail::WhenImpl(combinator, begin, count);
  return Future{std::move(future_core)};
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
template <FailPolicy P = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, It end) {
  static_assert(is_future_base_v<T>, "WhenAny function Iterator must be point to some Future");
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAny<P>(begin, static_cast<std::size_t>(end - begin));
}

/**
 * \brief Create \ref Future that is ready when any of futures is ready
 *
 * \tparam P policy WhenAny errors
 * \tparam V type of value all passed futures
 * \tparam E type of error all passed futures
 * \param futures two or more futures to combine
 * \return Future<T>
 */
template <FailPolicy P = FailPolicy::LastFail, typename V, typename... E>
auto WhenAny(FutureBase<V, E>&&... futures) {
  constexpr std::size_t kSize = sizeof...(E);
  static_assert(kSize >= 2, "WhenAny wants at least two futures");
  auto [future_core, combinator] = detail::AnyCombinator<V, head_t<E...>, P>::Make(kSize);
  detail::WhenImpl(*combinator, std::move(futures)...);
  return Future{std::move(future_core)};
}

}  // namespace yaclib
