#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename Waited, typename = std::enable_if_t<is_waitable_v<Waited>>>
YACLIB_INLINE auto AwaitInline(Waited& waited) noexcept {
  YACLIB_ASSERT(waited.Valid());
  return detail::AwaitAwaiter<typename Waited::Handle, false>{waited.GetHandle()};
}

template <typename... Waited, typename = std::enable_if_t<(... && is_waitable_v<Waited>)>>
YACLIB_INLINE auto AwaitInline(Waited&... waited) noexcept {
  using namespace detail;
  static constexpr auto kSharedCount = kCount<SharedHandle, typename Waited::Handle...>;
  using Awaiter = std::conditional_t<kSharedCount == 0, MultiAwaitAwaiter<AwaitEvent<false>>,
                                     MultiAwaitAwaiter<StaticSharedEvent<AwaitEvent<false>, kSharedCount>>>;
  YACLIB_ASSERT(... && waited.Valid());
  return Awaiter{waited.GetHandle()...};
}

template <typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type,
          typename = std::enable_if_t<is_waitable_v<Value>>>
YACLIB_INLINE auto AwaitInline(Iterator begin, std::size_t count) noexcept {
  using namespace detail;
  static constexpr auto kShared = std::is_same_v<typename Value::Handle, SharedHandle>;
  using Awaiter = std::conditional_t<kShared, MultiAwaitAwaiter<DynamicSharedEvent<AwaitEvent<false>>>,
                                     MultiAwaitAwaiter<AwaitEvent<false>>>;
  return Awaiter{begin, count};
}

template <typename Iterator,
          typename = std::enable_if_t<is_waitable_v<typename std::iterator_traits<Iterator>::value_type>>>
YACLIB_INLINE auto AwaitInline(Iterator begin, Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return AwaitInline(begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
