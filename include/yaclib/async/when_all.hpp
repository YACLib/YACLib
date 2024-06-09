#pragma once

#include <yaclib/async/detail/when_all_impl.hpp>
#include <yaclib/async/detail/when_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/order_policy.hpp>
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
 * \param begin , count the range of futures to combine
 * \return Future<std::vector<future_value_t<T>>, future_error_t<T>>
 */
template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
auto WhenAll(It begin, std::size_t count) {
  static_assert(F != FailPolicy::LastFail, "TODO(Ri7ay, MBkkt) Add other policy for WhenAll");
  static_assert(is_future_base_v<T>, "WhenAll function Iterator must be point to some Future");
  YACLIB_WARN(count < 2, "Don't use combinators for one or zero futures");
  using V = future_base_value_t<T>;
  using E = future_base_error_t<T>;
  using R = std::conditional_t<F == FailPolicy::None, Result<V, E>, V>;
  auto [future_core, combinator] = detail::AllCombinator<O, R, E>::Make(count);
  detail::WhenImpl(combinator, begin, count);
  return Future{std::move(future_core)};
}

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam It type of passed iterator
 * \tparam T type of all passed futures
 * \param begin , end the range of futures to combine
 * \return Future<std::vector<future_value_t<T>>, future_error_t<T>>
 */
template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, It end) {
  static_assert(F != FailPolicy::LastFail, "TODO(Ri7ay, MBkkt) Add other policy for WhenAll");
  static_assert(is_future_base_v<T>, "WhenAll function Iterator must be point to some Future");
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAll(begin, distance(begin, end))
  return WhenAll<F, O>(begin, static_cast<std::size_t>(end - begin));
}

/**
 * Create \ref Future which will be ready when all futures are ready
 *
 * \tparam V type of value all passed futures
 * \tparam E type of error all passed futures
 * \param futures two or more futures to combine
 * \return Future<std::array<T>>
 */
template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename V, typename... E>
auto WhenAll(FutureBase<V, E>&&... futures) {
  static_assert(F != FailPolicy::LastFail, "TODO(Ri7ay, MBkkt) Add other policy for WhenAll");
  constexpr std::size_t kSize = sizeof...(E);
  static_assert(kSize >= 2, "WhenAll wants at least two futures");
  using Error = head_t<E...>;
  using R = std::conditional_t<F == FailPolicy::None, Result<V, Error>, V>;
  auto [future_core, combinator] = detail::AllCombinator<O, R, Error>::Make(kSize);
  detail::WhenImpl(*combinator, std::move(futures)...);
  return Future{std::move(future_core)};
}

}  // namespace yaclib
