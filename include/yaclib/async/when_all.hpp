#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/detail/all.hpp>
#include <yaclib/async/detail/when.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/order_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename... Futures, typename = std::enable_if_t<(... && is_combinator_input_v<Futures>), void>>
YACLIB_INLINE auto Join(Futures... futures) {
  return detail::When<detail::Join>(std::move(futures)...);
}

template <typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto Join(It begin, std::size_t count) {
  return detail::When<detail::Join>(begin, count);
}

template <typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto Join(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAllVoid(begin, static_cast<std::size_t>(end - begin));
}

template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>), void>>
YACLIB_INLINE auto WhenAllVector(Futures... futures) {
  using Head = typename head_t<Futures...>::Core;
  using Value = typename head_t<Futures...>::Core::Value;
  using Error = typename head_t<Futures...>::Core::Error;
  static_assert((... && std::is_same_v<Value, typename Futures::Core::Value>));
  static_assert((... && std::is_same_v<Error, typename Futures::Core::Error>));

  static constexpr bool AllSame = (... && std::is_same_v<Head, typename Futures::Core>);
  using FinalCore = std::conditional_t<AllSame, Head, detail::ResultCore<Value, Error>>;

  using Strategy = detail::AllVector<F, O, FinalCore>;
  return detail::When<Strategy>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAllVector(It begin, std::size_t count) {
  using Strategy = detail::AllVector<F, O, typename T::Core>;
  return detail::When<Strategy>(begin, count);
}

template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAllVector(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAllVector<F, O>(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
