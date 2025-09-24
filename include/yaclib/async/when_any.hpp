#pragma once

#include <yaclib/async/detail/any.hpp>
#include <yaclib/async/detail/when.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::LastFail, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>), void>>
YACLIB_INLINE auto WhenAny(Futures... futures) {
  using Strategy = detail::Any<F, std::remove_reference_t<decltype(*futures.GetCore())>...>;
  return detail::When<Strategy>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, std::size_t count) {
  using Strategy = detail::Any<F, std::remove_reference_t<decltype(*begin->GetCore())>>;

  if constexpr (is_future_base_v<T>) {
    if (count == 1) {
      using V = future_base_value_t<T>;
      using E = future_base_error_t<T>;
      return Future<V, E>{std::exchange(begin->GetCore(), nullptr)};
    }
  }

  return detail::When<Strategy>(begin, count);
}

template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAny<F>(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
