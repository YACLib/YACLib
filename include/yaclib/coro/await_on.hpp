#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_on_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename Waited, typename = std::enable_if_t<is_waitable_v<Waited>>>
YACLIB_INLINE auto AwaitOn(IExecutor& e, Waited& waited) noexcept {
  YACLIB_ASSERT(waited.Valid());
  return detail::AwaitOnAwaiter{e, waited.GetHandle()};
}

template <typename... Waited, typename = std::enable_if_t<(... && is_waitable_v<Waited>)>>
YACLIB_INLINE auto AwaitOn(IExecutor& e, Waited&... waited) noexcept {
  using namespace detail;
  static constexpr auto kSharedCount = kCount<SharedHandle, typename Waited::Handle...>;
  using CoreEvent = AwaitOnEvent<false>;
  using Event = std::conditional_t<kSharedCount == 0, CoreEvent, StaticSharedEvent<CoreEvent, kSharedCount>>;
  YACLIB_ASSERT(... && waited.Valid());
  return MultiAwaitOnAwaiter<Event>{e, waited.GetHandle()...};
}

template <typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type,
          typename = std::enable_if_t<is_waitable_v<Value>>>
YACLIB_INLINE auto AwaitOn(IExecutor& e, Iterator begin, std::size_t count) noexcept {
  using namespace detail;
  static constexpr auto kShared = std::is_same_v<typename Value::Handle, SharedHandle>;
  using CoreEvent = AwaitOnEvent<false>;
  using Event = std::conditional_t<kShared, DynamicSharedEvent<CoreEvent>, CoreEvent>;
  return MultiAwaitOnAwaiter<Event>{e, begin, count};
}

template <typename Iterator,
          typename = std::enable_if_t<is_waitable_v<typename std::iterator_traits<Iterator>::value_type>>>
YACLIB_INLINE auto AwaitOn(IExecutor& e, Iterator begin, Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return AwaitOn(e, begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
