#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/detail/all.hpp>
#include <yaclib/async/detail/all_tuple.hpp>
#include <yaclib/async/detail/when.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename Future, FailPolicy F>
using TupleElem = std::conditional_t<F == FailPolicy::FirstFail, wrap_void_t<typename Future::Core::Value>,
                                     typename Future::Core::ResultType>;

template <FailPolicy F = FailPolicy::FirstFail, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto WhenAll(Futures... futures) {
  using Head = typename head_t<Futures...>::Core;
  using Value = typename Head::Value;
  using OutputError = typename Head::Error;
  static_assert((... && std::is_same_v<OutputError, typename Futures::Core::Error>),
                "All futures should have the same error type");

  if constexpr ((... && std::is_same_v<Value, typename Futures::Core::Value>)) {
    using OutputValue = std::vector<wrap_void_t<Value>>;
    return detail::When<detail::All, F, OutputValue, OutputError>(std::move(futures)...);
  } else {
    using OutputValue = std::tuple<TupleElem<Futures, F>...>;
    return detail::When<detail::AllTuple, F, OutputValue, OutputError>(std::move(futures)...);
  }
}

template <FailPolicy F = FailPolicy::FirstFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, std::size_t count) {
  using OutputValue = std::vector<wrap_void_t<typename T::Core::Value>>;
  using OutputError = typename T::Core::Error;

  return detail::When<detail::All, F, OutputValue, OutputError>(begin, count);
}

template <FailPolicy F = FailPolicy::FirstFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAll<F>(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
