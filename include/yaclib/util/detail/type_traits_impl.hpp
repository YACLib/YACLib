#pragma once

#include <yaclib/fwd.hpp>

#include <iterator>
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
struct InstantiationType final {
  using Value = T;
};

template <template <typename...> typename Instance, typename V>
struct InstantiationType<Instance, Instance<V>> final {
  using Value = V;
};

template <template <typename...> typename Instance, typename T>
struct InstantiationTypes final {
  using Value = T;
  using Trait = T;
};

template <template <typename...> typename Instance, typename V, typename T>
struct InstantiationTypes<Instance, Instance<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename T>
struct AsyncTypes final {
  using Value = T;
  using Trait = T;
};

template <typename V, typename T>
struct AsyncTypes<FutureBase<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename V, typename T>
struct AsyncTypes<Future<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename V, typename T>
struct AsyncTypes<FutureOn<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename V, typename T>
struct AsyncTypes<SharedFutureBase<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename V, typename T>
struct AsyncTypes<SharedFuture<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename V, typename T>
struct AsyncTypes<SharedFutureOn<V, T>> final {
  using Value = V;
  using Trait = T;
};

template <typename It, typename Tag, typename = std::void_t<>>
struct HasIteratorCategory {
  static constexpr bool Value = false;
};

template <typename It, typename Tag>
struct HasIteratorCategory<It, Tag, std::void_t<typename std::iterator_traits<It>::iterator_category>> final {
  static constexpr bool Value = std::is_base_of_v<Tag, typename std::iterator_traits<It>::iterator_category>;
};

template <typename It, typename = std::void_t<>>
struct IsInputIterator {
  static constexpr bool Value = false;
};

template <typename It>
struct IsInputIterator<It, std::void_t<decltype(*std::declval<const It&>()), decltype(++std::declval<It&>())>> final {
  static constexpr bool Value = HasIteratorCategory<It, std::input_iterator_tag>::Value;
};

template <typename Sentinel, typename It, typename = std::void_t<>>
struct IsSentinelFor {
  static constexpr bool Value = false;
};

template <typename Sentinel, typename It>
struct IsSentinelFor<Sentinel, It,
                     std::void_t<decltype(std::declval<const It&>() == std::declval<const Sentinel&>())>> {
  static constexpr bool Value = true;
};

template <typename It, typename Sentinel>
struct IsInputRangePair {
  static constexpr bool Value = IsInputIterator<It>::Value && IsSentinelFor<Sentinel, It>::Value;
};

template <typename Range>
using RangeIterator = std::decay_t<decltype(std::begin(std::declval<Range&>()))>;

template <typename Range>
using RangeSentinel = std::decay_t<decltype(std::end(std::declval<Range&>()))>;

template <typename Range, typename = std::void_t<>>
struct IsInputRange {
  static constexpr bool Value = false;
};

template <typename Range>
struct IsInputRange<Range, std::void_t<RangeIterator<Range>, RangeSentinel<Range>>> {
  static constexpr bool Value = IsInputRangePair<RangeIterator<Range>, RangeSentinel<Range>>::Value;
};

template <typename It, typename Sentinel, typename = std::void_t<>>
struct HasConstantTimeDistance {
  static constexpr bool Value = false;
};

template <typename It, typename Sentinel>
struct HasConstantTimeDistance<It, Sentinel,
                               std::void_t<decltype(std::declval<const Sentinel&>() - std::declval<const It&>()),
                                           decltype(std::declval<const It&>() - std::declval<const Sentinel&>())>> {
  static constexpr bool Value = true;
};

}  // namespace yaclib::detail
