#pragma once

#include <yaclib/fwd.hpp>

#include <type_traits>

namespace yaclib::detail {

template <typename...>
struct Head;

template <typename T, typename... Args>
struct Head<T, Args...> {
  using Type = T;
};

template <typename Func, typename... Args>
struct IsInvocable {
  static constexpr bool Value = std::is_invocable_v<Func, Args...>;
};

template <typename Func>
struct IsInvocable<Func, void> {
  static constexpr bool Value = std::is_invocable_v<Func>;
};

template <typename Func, typename... Args>
struct Invoke {
  using Type = std::invoke_result_t<Func, Args...>;
};

template <typename Func>
struct Invoke<Func, void> {
  using Type = std::invoke_result_t<Func>;
};

template <template <typename...> typename Instance, typename...>
struct IsInstantiationOf {
  static constexpr bool Value = false;
};

template <template <typename...> typename Instance, typename... Args>
struct IsInstantiationOf<Instance, Instance<Args...>> {
  static constexpr bool Value = true;
};

template <template <typename...> typename Instance, typename T>
struct InstantiationTypes {
  using Value = T;
  using Error = T;
};

template <template <typename...> typename Instance, typename V, typename E>
struct InstantiationTypes<Instance, Instance<V, E>> {
  using Value = V;
  using Error = E;
};

template <typename T>
struct FutureBaseTypes {
  using Value = T;
  using Error = T;
};

template <typename V, typename E>
struct FutureBaseTypes<FutureBase<V, E>> {
  using Value = V;
  using Error = E;
};

template <typename V, typename E>
struct FutureBaseTypes<Future<V, E>> {
  using Value = V;
  using Error = E;
};

template <typename V, typename E>
struct FutureBaseTypes<FutureOn<V, E>> {
  using Value = V;
  using Error = E;
};

}  // namespace yaclib::detail
