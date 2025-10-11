#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/when/all.hpp>
#include <yaclib/async/when/all_tuple.hpp>
#include <yaclib/async/when/join.hpp>
#include <yaclib/async/when/when.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename Core, FailPolicy F>
using ContainerElem = std::conditional_t<F == FailPolicy::FirstFail, wrap_void_t<typename Core::Value>,
                                         Result<typename Core::Value, typename Core::Error>>;

template <FailPolicy F = FailPolicy::FirstFail, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto WhenAll(Futures... futures) {
  when::CheckSameError<Futures...>();

  using Head = typename head_t<Futures...>::Core;
  using Value = typename Head::Value;
  using OutputError = typename Head::Error;

  if constexpr ((... && std::is_same_v<Value, typename Futures::Core::Value>)) {
    if constexpr (std::is_same_v<Value, void> && F != FailPolicy::None) {
      return when::When<when::Join, F, void, OutputError>(std::move(futures)...);
    } else {
      using OutputValue = std::vector<ContainerElem<Head, F>>;
      return when::When<when::All, F, OutputValue, OutputError>(std::move(futures)...);
    }
  } else {
    using OutputValue = std::tuple<ContainerElem<typename Futures::Core, F>...>;
    return when::When<when::AllTuple, F, OutputValue, OutputError>(std::move(futures)...);
  }
}

template <FailPolicy F = FailPolicy::FirstFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, std::size_t count) {
  using OutputError = typename T::Core::Error;

  if constexpr (std::is_same_v<typename T::Core::Value, void> && F != FailPolicy::None) {
    return when::When<when::Join, F, void, OutputError>(begin, count);
  } else {
    using OutputValue = std::vector<ContainerElem<typename T::Core, F>>;
    return when::When<when::All, F, OutputValue, OutputError>(begin, count);
  }
}

template <FailPolicy F = FailPolicy::FirstFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAll<F>(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
