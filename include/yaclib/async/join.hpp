#pragma once

#include <yaclib/async/when/join.hpp>
#include <yaclib/async/when/when.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::None, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto Join(Futures... futures) {
  when::CheckSameError<Futures...>();
  return when::When<when::Join, F, void, typename head_t<Futures...>::Core::Error>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::None, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto Join(It begin, std::size_t count) {
  return when::When<when::Join, F, void, typename T::Core::Error>(begin, count);
}

template <FailPolicy F = FailPolicy::None, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto Join(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return Join<F>(begin, static_cast<std::size_t>(end - begin));
}

};  // namespace yaclib
