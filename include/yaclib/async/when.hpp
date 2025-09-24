#pragma once

#include <yaclib/async/detail/all.hpp>
#include <yaclib/async/detail/any.hpp>
#include <yaclib/async/detail/when.hpp>
#include <yaclib/async/future.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::LastFail, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>), void>>
YACLIB_INLINE auto WhenAny(Futures... futures) {
  using Strategy = detail::Any<F, std::remove_reference_t<decltype(*futures.GetCore())>...>;
  return detail::When<Strategy>(*futures.GetCore().Release()...);
}

template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, std::size_t count) {
  using Strategy = detail::Any<F, std::remove_reference_t<decltype(*begin->GetCore())>>;

  if (count == 1) {
    using V = future_base_value_t<T>;
    using E = future_base_error_t<T>;
    return Future<V, E>{std::exchange(begin->GetCore(), nullptr)};
  }

  return detail::When<Strategy>(begin, count);
}

template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAny<F>(begin, static_cast<std::size_t>(end - begin));
}

// template <FailPolicy F = FailPolicy::LastFail, typename... Futures,
//           typename = std::enable_if_t<(... && is_waitable_v<Futures>), void>>
// YACLIB_INLINE auto WhenAllVoid(Futures&&... futures) {
//   using Strategy = detail::AllVoid<F, std::remove_reference_t<decltype(*futures.GetCore())>...>;
//   return detail::When<Strategy>(*futures.GetCore().Release()...);
// }

// template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename
// std::iterator_traits<It>::value_type> YACLIB_INLINE auto WhenAllVoid(It begin, std::size_t count) {
//   using Strategy = detail::AllVoid<F, std::remove_reference_t<decltype(*begin->GetCore())>>;
//   return detail::When<Strategy>(begin, count);
// }

// template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename
// std::iterator_traits<It>::value_type> YACLIB_INLINE auto WhenAllVoid(It begin, It end) {
//   // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
//   // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
//   return WhenAllVoid<F>(begin, static_cast<std::size_t>(end - begin));
// }

template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>), void>>
YACLIB_INLINE auto WhenAllVector(Futures... futures) {
  using Value = typename head_t<Futures...>::Core::Value;
  using Error = typename head_t<Futures...>::Core::Error;
  static_assert((... && std::is_same_v<Value, typename Futures::Core::Value>));
  static_assert((... && std::is_same_v<Error, typename Futures::Core::Error>));

  using Strategy = detail::AllVector<F, O, detail::ResultCore<Value, Error>>;
  return detail::When<Strategy>(*futures.GetCore().Release()...);
}

template <FailPolicy F = FailPolicy::FirstFail, OrderPolicy O = OrderPolicy::Fifo, typename It,
          typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAllVector(It begin, std::size_t count) {
  using Value = typename T::Core::Value;
  using Error = typename T::Core::Error;

  using Strategy = detail::AllVector<F, O, detail::ResultCore<Value, Error>>;
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
