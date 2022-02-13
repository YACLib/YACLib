#pragma once

#include <type_traits>

namespace yaclib ::detail {

template <typename Functor, typename... Arg>
struct IsInvocable {
  static constexpr bool Value = std::is_invocable_v<Functor, Arg...>;
};

template <typename Functor>
struct IsInvocable<Functor, void> {
  static constexpr bool Value = std::is_invocable_v<Functor>;
};

template <typename Functor, typename... Arg>
struct Invoke {
  using Type = std::invoke_result_t<Functor, Arg...>;
};

template <typename Functor>
struct Invoke<Functor, void> {
  using Type = std::invoke_result_t<Functor>;
};

template <template <typename...> typename T, typename...>
struct IsInstantiationOf {
  static constexpr bool Value = false;
};

template <template <typename...> typename T, typename... U>
struct IsInstantiationOf<T, T<U...>> {
  static constexpr bool Value = true;
};

template <template <typename...> typename T, typename V>
struct InstantiationTypes {
  using Value = V;
  using Error = V;
};

template <template <typename...> typename T, typename V, typename E>
struct InstantiationTypes<T, T<V, E>> {
  using Value = V;
  using Error = E;
};

}  // namespace yaclib::detail
