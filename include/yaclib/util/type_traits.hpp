#pragma once

#include <type_traits>

namespace yaclib {
namespace async {

template <typename T>
class Future;

}  // namespace async
namespace util {

template <template <typename...> class T, typename... U>
struct IsInstantiationOf : std::false_type {};

template <template <typename...> class T, typename... U>
struct IsInstantiationOf<T, T<U...>> : std::true_type {};

template <typename Functor, typename... Arg>
struct Invoke {
  using type = std::invoke_result_t<Functor, Arg...>;
};

template <typename Functor>
struct Invoke<Functor, void> {
  using type = std::invoke_result_t<Functor>;
};

template <typename Functor, typename... Arg>
using InvokeT = typename Invoke<Functor, Arg...>::type;

template <typename Functor, typename Arg>
struct IsInvocable {
  static constexpr bool value = std::is_invocable_v<Functor, Arg>;
};

template <typename Functor>
struct IsInvocable<Functor, void> {
  static constexpr bool value = std::is_invocable_v<Functor>;
};

template <typename Functor, typename Arg>
inline constexpr bool IsInvocableV = IsInvocable<Functor, Arg>::value;

template <typename T>
using DecayT = std::remove_reference_t<std::decay_t<T>>;

template <typename T>
class Result;

template <typename T>
inline constexpr bool IsFutureV = std::integral_constant<bool, IsInstantiationOf<async::Future, T>::value>::value;

template <typename T>
inline constexpr bool IsResultV = std::integral_constant<bool, IsInstantiationOf<Result, T>::value>::value;

}  // namespace util
}  // namespace yaclib
