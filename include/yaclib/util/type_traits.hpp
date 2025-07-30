#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/detail/type_traits_impl.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace yaclib {

// Not available in C++17, move to std if C++17 support is ever dropped
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename... Args>
using head_t = typename detail::Head<Args...>::Type;  // NOLINT

template <typename Func, typename... Arg>
inline constexpr bool is_invocable_v = detail::IsInvocable<Func, Arg...>::Value;  // NOLINT

template <typename Func, typename... Arg>
using invoke_t = typename detail::Invoke<Func, Arg...>::Type;  // NOLINT

template <typename T>
inline constexpr bool is_result_v = detail::IsInstantiationOf<Result, T>::Value;  // NOLINT

template <typename T>
using result_value_t = typename detail::InstantiationTypes<Result, T>::Value;  // NOLINT

template <typename T>
using result_error_t = typename detail::InstantiationTypes<Result, T>::Error;  // NOLINT

template <typename T>
using task_value_t = typename detail::InstantiationTypes<Task, T>::Value;  // NOLINT

template <typename T>
using task_error_t = typename detail::InstantiationTypes<Task, T>::Error;  // NOLINT

template <typename T>
inline constexpr bool is_future_base_v = detail::IsInstantiationOf<FutureBase, T>::Value ||  // NOLINT
                                         detail::IsInstantiationOf<Future, T>::Value ||      // dummy comments
                                         detail::IsInstantiationOf<FutureOn, T>::Value;      // for format
template <typename T>
inline constexpr bool is_task_v = detail::IsInstantiationOf<Task, T>::Value;  // NOLINT

template <typename T>
inline constexpr bool is_shared_future_v = detail::IsInstantiationOf<SharedFuture, T>::Value;

// Waitable: a reference to a shared future or a non-const reference to a regular future
template <typename T>
inline constexpr bool is_waitable_v =
  is_shared_future_v<remove_cvref_t<T>> ||
  (!std::is_const_v<std::remove_reference_t<T>> && is_future_base_v<remove_cvref_t<T>>);

// Waitable with timeout: a non-const reference to a regular future (shared futures cannot be waited with timeout)
template <typename T>
inline constexpr bool is_waitable_with_timeout_v =
  (!std::is_const_v<std::remove_reference_t<T>> && is_future_base_v<remove_cvref_t<T>>);

template <typename T>
using future_base_value_t = typename detail::FutureBaseTypes<T>::Value;  // NOLINT

template <typename T>
using future_base_error_t = typename detail::FutureBaseTypes<T>::Error;  // NOLINT

template <bool Condition, typename T>
decltype(auto) move_if(T&& arg) noexcept {  // NOLINT
  if constexpr (Condition) {
    return std::move(std::forward<T>(arg));
  } else {
    return std::forward<T>(arg);
  }
}

template <typename T>
constexpr bool Check() noexcept {
  static_assert(!std::is_reference_v<T>, "T cannot be V&, just use pointer or std::reference_wrapper");
  static_assert(!std::is_const_v<T>, "T cannot be const, because it's unnecessary");
  static_assert(!std::is_volatile_v<T>, "T cannot be volatile, because it's unnecessary");
  static_assert(!is_result_v<T>, "T cannot be Result, because it's ambiguous");
  static_assert(!is_future_base_v<T>, "T cannot be Future, because it's ambiguous");
  static_assert(!is_task_v<T>, "T cannot be Task, because it's ambiguous");
  static_assert(!std::is_same_v<T, std::exception_ptr>, "T cannot be std::exception_ptr, because it's ambiguous");
  static_assert(!std::is_same_v<T, Unit>, "T cannot be Unit, because Unit for internal instead of void usage");
  return true;
}

}  // namespace yaclib
