#pragma once

#include <yaclib/async/when/any.hpp>
#include <yaclib/async/when/when.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/fail_policy.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <FailPolicy F = FailPolicy::LastFail, typename... Futures,
          typename = std::enable_if_t<(... && is_combinator_input_v<Futures>)>>
YACLIB_INLINE auto WhenAny(Futures... futures) {
  when::CheckSameTrait<Futures...>();

  using OutputValue = typename MaybeVariant<typename Unique<std::tuple<typename Futures::Core::Value...>>::Type>::Type;
  using OutputTrait = typename head_t<Futures...>::Core::Trait;

  return when::When<when::Any, F, OutputValue, OutputTrait>(std::move(futures)...);
}

template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, std::size_t count) {
  if constexpr (is_future_base_v<T>) {
    if (count == 1) {
      using V = async_value_t<T>;
      using Trait = async_trait_t<T>;
      return Future<V, Trait>{std::exchange(begin->GetCore(), nullptr)};
    }
  }

  return when::When<when::Any, F, typename T::Core::Value, typename T::Core::Trait>(begin, count);
}

template <FailPolicy F = FailPolicy::LastFail, typename It, typename T = typename std::iterator_traits<It>::value_type>
YACLIB_INLINE auto WhenAny(It begin, It end) {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call WhenAny(begin, distance(begin, end))
  return WhenAny<F>(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
