#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/detail/all.hpp>
#include <yaclib/async/detail/when.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::FirstFail, template <typename...> typename Container = std::vector,
          typename... Futures, typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto WhenAll(Futures... futures) {
  using Head = typename head_t<Futures...>::Core;
  using Value = typename Head::Value;
  using Error = typename Head::Error;
  static_assert((... && std::is_same_v<Value, typename Futures::Core::Value>));
  static_assert((... && std::is_same_v<Error, typename Futures::Core::Error>));

  static constexpr bool kAllSame = (... && std::is_same_v<Head, typename Futures::Core>);
  using FinalCore = std::conditional_t<kAllSame, Head, detail::ResultCore<Value, Error>>;

  using Strategy = detail::All<F, detail::ElementType::Value, Container, FinalCore>;
  return detail::When<Strategy>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::FirstFail, template <typename...> typename Container = std::vector, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, std::size_t count) {
  using Strategy = detail::All<F, detail::ElementType::Value, Container, typename T::Core>;
  return detail::When<Strategy>(begin, count);
}

template <FailPolicy F = FailPolicy::FirstFail, template <typename...> typename Container = std::vector, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAll(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAll<F, Container>(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
