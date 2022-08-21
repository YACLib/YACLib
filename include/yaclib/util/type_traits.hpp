#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/detail/type_traits_impl.hpp>

#include <exception>
#include <type_traits>

namespace yaclib {

template <typename... Args>
using head_t = typename detail::Head<Args...>::Type;

template <typename Func, typename... Arg>
inline constexpr bool is_invocable_v = detail::IsInvocable<Func, Arg...>::Value;

template <typename Func, typename... Arg>
using invoke_t = typename detail::Invoke<Func, Arg...>::Type;

template <typename T>
inline constexpr bool is_result_v = detail::IsInstantiationOf<Result, T>::Value;

template <typename T>
using result_value_t = typename detail::InstantiationTypes<Result, T>::Value;

template <typename T>
using result_error_t = typename detail::InstantiationTypes<Result, T>::Error;

template <typename T>
inline constexpr bool is_future_base_v = detail::IsInstantiationOf<FutureBase, T>::Value ||  //
                                         detail::IsInstantiationOf<Future, T>::Value ||      //
                                         detail::IsInstantiationOf<FutureOn, T>::Value;
template <typename T>
inline constexpr bool is_task_v = detail::IsInstantiationOf<Task, T>::Value;

template <typename T>
using future_base_value_t = typename detail::FutureBaseTypes<T>::Value;

template <typename T>
using future_base_error_t = typename detail::FutureBaseTypes<T>::Error;

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
