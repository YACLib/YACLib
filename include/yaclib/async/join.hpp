#pragma once

#include <yaclib/async/detail/join.hpp>
#include <yaclib/async/detail/when.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::None, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto Join(Futures... futures) {
  using OutputError = typename head_t<Futures...>::Core::Error;
  static_assert((... && std::is_same_v<OutputError, typename Futures::Core::Error>),
                "All futures need to have the same error type");

  return detail::When<detail::Join, F, void, OutputError>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::None, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto Join(It begin, std::size_t count) {
  return detail::When<detail::Join, F, void, typename T::Core::Error>(begin, count);
}

template <FailPolicy F = FailPolicy::None, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto Join(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return Join<F>(begin, static_cast<std::size_t>(end - begin));
}

};  // namespace yaclib
