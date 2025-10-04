#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/detail/type_traits_impl.hpp>

#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

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
inline constexpr bool is_shared_future_base_v = detail::IsInstantiationOf<SharedFutureBase, T>::Value ||  // NOLINT
                                                detail::IsInstantiationOf<SharedFuture, T>::Value ||  // dummy comments
                                                detail::IsInstantiationOf<SharedFutureOn, T>::Value;  // for format

template <typename T>
inline constexpr bool is_task_v = detail::IsInstantiationOf<Task, T>::Value;  // NOLINT

// Waitable: a reference to a shared future or a non-const reference to a regular future
template <typename T>
inline constexpr bool is_waitable_v =
  is_shared_future_base_v<remove_cvref_t<T>> ||
  (!std::is_const_v<std::remove_reference_t<T>> && is_future_base_v<remove_cvref_t<T>>);

// Waitable with timeout: a non-const reference to a regular future (shared futures cannot be waited with timeout)
template <typename T>
inline constexpr bool is_waitable_with_timeout_v =
  (!std::is_const_v<std::remove_reference_t<T>> && is_future_base_v<remove_cvref_t<T>>);

// Combinator input: futures by value
template <typename T>
inline constexpr bool is_combinator_input_v = (is_shared_future_base_v<T> || is_future_base_v<T>);

template <typename T>
using async_value_t = typename detail::AsyncTypes<T>::Value;  // NOLINT

template <typename T>
using async_error_t = typename detail::AsyncTypes<T>::Error;  // NOLINT

template <bool Condition, typename T>
decltype(auto) move_if(T&& arg) noexcept {  // NOLINT
  if constexpr (Condition) {
    return std::move(std::forward<T>(arg));
  } else {
    return std::forward<T>(arg);
  }
}

template <typename T, typename... List>
inline constexpr auto kCount = (std::size_t{std::is_same_v<T, List> ? 1 : 0} + ...);

template <typename T, typename... Ts>
inline constexpr auto kContains = (std::is_same_v<T, Ts> || ...);

template <typename T, typename Tuple>
struct Prepend;

template <typename T, typename... Ts>
struct Prepend<T, std::tuple<Ts...>> {
  using Type = std::tuple<T, Ts...>;
};

template <typename Tuple>
struct Tail;

template <typename T, typename... Ts>
struct Tail<std::tuple<T, Ts...>> {
  using Type = std::tuple<Ts...>;
};

template <typename Tuple>
using tail_t = typename Tail<Tuple>::Type;

template <template <typename> typename F, typename Tuple>
struct Filter;

template <template <typename> typename F>
struct Filter<F, std::tuple<>> {
  using Type = std::tuple<>;
};

template <template <typename> typename F, typename T>
struct Filter<F, std::tuple<T>> {
  using Type = std::conditional_t<F<T>::Value, std::tuple<T>, std::tuple<>>;
};

template <template <typename> typename F, typename T, typename... Ts>
struct Filter<F, std::tuple<T, Ts...>> {
 private:
  using PrevType = typename Filter<F, std::tuple<Ts...>>::Type;

 public:
  using Type = std::conditional_t<F<T>::Value, typename Prepend<T, PrevType>::Type, PrevType>;
};

template <typename Tuple>
struct Unique;

template <>
struct Unique<std::tuple<>> {
  using Type = std::tuple<>;
};

template <typename T>
struct Unique<std::tuple<T>> {
  using Type = std::tuple<T>;
};

template <typename T, typename... Ts>
struct Unique<std::tuple<T, Ts...>> {
 private:
  using PrevType = typename Unique<std::tuple<Ts...>>::Type;

 public:
  using Type = std::conditional_t<kContains<T, Ts...>, PrevType, typename Prepend<T, PrevType>::Type>;
};

template <typename Tuple>
struct Variant;

template <typename... Ts>
struct Variant<std::tuple<Ts...>> {
  using Type = std::variant<Ts...>;
};

template <typename Tuple>
struct MaybeVariant;

template <typename T>
struct MaybeVariant<std::tuple<T>> {
  using Type = T;
};

template <typename... Ts>
struct MaybeVariant<std::tuple<Ts...>> {
  using Type = std::variant<Ts...>;
};

template <typename T>
struct WrapVoid {
  using Type = T;
};

template <>
struct WrapVoid<void> {
  using Type = Unit;
};

template <typename T>
using wrap_void_t = typename WrapVoid<T>::Type;

template <size_t FromIndex, size_t ToIndex, typename FromTuple, typename ToTuple>
struct TranslateIndexImpl;

template <size_t ToIndex, typename... From, typename... To>
struct TranslateIndexImpl<0, ToIndex, std::tuple<From...>, std::tuple<To...>> {
  static_assert(sizeof...(From) >= sizeof...(To));
  static constexpr size_t Index() {
    return ToIndex;
  }
};

template <size_t FromIndex, size_t ToIndex, typename... From, typename... To>
struct TranslateIndexImpl<FromIndex, ToIndex, std::tuple<From...>, std::tuple<To...>> {
  static_assert(sizeof...(From) >= sizeof...(To));
  static_assert(FromIndex != 0);
  static constexpr size_t Index() {
    if constexpr (std::is_same_v<head_t<From...>, head_t<To...>>) {
      return TranslateIndexImpl<FromIndex - 1, ToIndex + 1, tail_t<std::tuple<From...>>,
                                tail_t<std::tuple<To...>>>::Index();
    } else {
      return TranslateIndexImpl<FromIndex - 1, ToIndex, tail_t<std::tuple<From...>>, std::tuple<To...>>::Index();
    }
  }
};

template <size_t FromIndex, typename FromTuple, typename ToTuple>
inline constexpr size_t translate_index_v = TranslateIndexImpl<FromIndex, 0, FromTuple, ToTuple>::Index();

template <typename T, typename Tuple>
struct IndexOf;

template <typename T, typename... Ts>
struct IndexOf<T, std::tuple<Ts...>> {
  static_assert(sizeof...(Ts) > 0);
  static constexpr size_t Index() {
    if constexpr (std::is_same_v<T, head_t<Ts...>>) {
      return 0;
    } else {
      return 1 + IndexOf<T, tail_t<std::tuple<Ts...>>>::Index();
    }
  }
};

template <typename T, typename Tuple>
inline constexpr size_t index_of_v = IndexOf<T, Tuple>::Index();

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
