#pragma once
#include <cstdint>
#include <type_traits>

namespace yaclib::detail {

template <typename Functor, typename PrevProxy>
class LazyProxy;

template <typename Functor, typename PrevP, typename Ret>
class ReversedLazyProxy;

struct Nil {
  using FunctorT = void;
  using ReturnType = void;
};

template <typename P>
struct GetProxySize;

template <>
struct GetProxySize<Nil> {
  constexpr static size_t value = 0;
};

template <typename Functor, typename PrevProxy>
struct GetProxySize<LazyProxy<Functor, PrevProxy>> {
  constexpr static size_t value = 1 + GetProxySize<PrevProxy>::value;
};

template <typename F, typename PrevProxy, typename Ret>
struct GetProxySize<ReversedLazyProxy<F, PrevProxy, Ret>> {
  constexpr static size_t value = 1 + GetProxySize<PrevProxy>::value;
};

template <typename Lazy, typename... R>
struct GetLazyTypes;

template <typename... R>
struct GetLazyTypes<Nil, R...> {
  using type = std::variant<R...>;
};

template <typename Lazy, typename... R>
struct GetLazyTypes {
  using type =
      std::conditional_t<!std::is_same_v<typename Lazy::ReturnType, void>,
                         typename GetLazyTypes<typename Lazy::PrevProxy, R..., typename Lazy::ReturnType>::type,
                         typename GetLazyTypes<typename Lazy::PrevProxy, R..., std::monostate>::type>;
};

}  // namespace yaclib::detail
