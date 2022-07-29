#pragma once

#include <yaclib/fwd.hpp>

#include <type_traits>

namespace yaclib::detail {

template <typename...>
struct Head;

template <typename T, typename... Args>
struct Head<T, Args...> final {
  using Type = T;
};

template <typename Func, typename... Args>
struct IsInvocable final {
  static constexpr bool Value = std::is_invocable_v<Func, Args...>;
};

template <typename Func>
struct IsInvocable<Func, void> final {
  static constexpr bool Value = std::is_invocable_v<Func>;
};

template <typename Func, typename... Args>
struct Invoke final {
  using Type = std::invoke_result_t<Func, Args...>;
};

template <typename Func>
struct Invoke<Func, void> final {
  using Type = std::invoke_result_t<Func>;
};

template <template <typename...> typename Instance, typename...>
struct IsInstantiationOf final {
  static constexpr bool Value = false;
};

template <template <typename...> typename Instance, typename... Args>
struct IsInstantiationOf<Instance, Instance<Args...>> final {
  static constexpr bool Value = true;
};

template <template <typename...> typename Instance, typename T>
struct InstantiationTypes final {
  using Value = T;
  using Error = T;
};

template <template <typename...> typename Instance, typename V, typename E>
struct InstantiationTypes<Instance, Instance<V, E>> final {
  using Value = V;
  using Error = E;
};

template <typename T>
struct FutureBaseTypes final {
  using Value = T;
  using Error = T;
};

template <typename V, typename E>
struct FutureBaseTypes<FutureBase<V, E>> final {
  using Value = V;
  using Error = E;
};

template <typename V, typename E>
struct FutureBaseTypes<Future<V, E>> final {
  using Value = V;
  using Error = E;
};

template <typename V, typename E>
struct FutureBaseTypes<FutureOn<V, E>> final {
  using Value = V;
  using Error = E;
};

}  // namespace yaclib::detail
