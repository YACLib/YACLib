#pragma once

#include <yaclib/async/when/join.hpp>
#include <yaclib/async/when/when.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::None, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto Join(Futures... futures) {
  when::CheckSameTrait<Futures...>();
  return when::When<when::Join, F, void, typename head_t<Futures...>::Core::Trait>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::None, typename It, typename = std::enable_if_t<is_input_iterator_v<It>>>
YACLIB_INLINE auto Join(It begin, std::size_t count) {
  using T = typename std::iterator_traits<It>::value_type;
  return when::When<when::Join, F, void, typename T::Core::Trait>(begin, count);
}

template <FailPolicy F = FailPolicy::None, typename It, typename Sentinel,
          typename = std::enable_if_t<is_input_range_pair_v<It, Sentinel>>>
YACLIB_INLINE auto Join(It begin, Sentinel end) {
  static_assert(
    has_constant_time_distance_v<It, Sentinel>,
    "Use Join(begin, std::distance(begin, end)) instead");  // We don't use std::distance because we want to alert
                                                            // the user to the fact that it can be expensive.

  return Join<F>(begin, static_cast<std::size_t>(end - begin));
}

template <FailPolicy F = FailPolicy::None, typename Range, typename = std::enable_if_t<is_input_range_v<Range>>>
YACLIB_INLINE auto Join(Range&& range) {
  return Join<F>(std::begin(range), std::end(range));
}

};  // namespace yaclib
