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
  using Trait = T;
};

template <template <typename...> typename Instance, typename V>
struct InstantiationTypes<Instance, Instance<V>> final {
  using Value = V;
  using Trait = void;
};

template <template <typename...> typename Instance, typename V, typename T>
struct InstantiationTypes<Instance, Instance<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename T>
struct FutureBaseTypes final {
  using Value = T;
  using Trait = T;
};

template <typename V, typename T>
struct FutureBaseTypes<FutureBase<V, T>> final {
  using Value = V;
  using Error = T;
};

template <typename V, typename T>
struct FutureBaseTypes<Future<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename V, typename T>
struct FutureBaseTypes<FutureOn<V, T>> final {
  using Value = V;
  using Trait = T;
};

}  // namespace yaclib::detail
