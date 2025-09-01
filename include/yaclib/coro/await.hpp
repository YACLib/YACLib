#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename E>
YACLIB_INLINE auto Await(Task<V, E>& task) noexcept {
  YACLIB_ASSERT(task.Valid());
  return detail::TransferAwaiter{UpCast<detail::BaseCore>(*task.GetCore())};
}

template <typename Waited, typename = std::enable_if_t<is_waitable_v<Waited>>>
YACLIB_INLINE auto Await(Waited& waited) noexcept {
  YACLIB_ASSERT(waited.Valid());
  return detail::AwaitAwaiter{waited.GetHandle()};
}

template <typename... Waited, typename = std::enable_if_t<(... && is_waitable_v<Waited>)>>
YACLIB_INLINE auto Await(Waited&... waited) noexcept {
  using namespace detail;
  static constexpr size_t SharedCount = Count<SharedHandle, typename Waited::Handle...>;
  using Awaiter = std::conditional_t<SharedCount == 0, MultiAwaitAwaiter<AwaitEvent>,
                                     MultiAwaitAwaiter<StaticSharedEvent<AwaitEvent, SharedCount>>>;
  YACLIB_ASSERT(... && waited.Valid());
  return Awaiter{waited.GetHandle()...};
}

template <typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type,
          typename = std::enable_if_t<is_waitable_v<Value>>>
YACLIB_INLINE auto Await(Iterator begin, std::size_t count) noexcept {
  using namespace detail;
  static constexpr bool Shared = std::is_same_v<typename Value::Handle, SharedHandle>;
  using Awaiter =
    std::conditional_t<Shared, MultiAwaitAwaiter<DynamicSharedEvent<AwaitEvent>>, MultiAwaitAwaiter<AwaitEvent>>;
  return Awaiter{begin, count};
}

template <typename Iterator,
          typename = std::enable_if_t<is_waitable_v<typename std::iterator_traits<Iterator>::value_type>>>
YACLIB_INLINE auto Await(Iterator begin, Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return Await(begin, static_cast<std::size_t>(end - begin));
}

template <typename V, typename E>
YACLIB_INLINE auto operator co_await(FutureBase<V, E>&& future) noexcept {
  YACLIB_ASSERT(future.Valid());
  return detail::AwaitSingleAwaiter<false, V, E>{std::move(future.GetCore())};
}

template <typename V, typename E>
YACLIB_INLINE auto operator co_await(const SharedFutureBase<V, E>& future) noexcept {
  YACLIB_ASSERT(future.Valid());
  return detail::AwaitSingleAwaiter<true, V, E>{future.GetCore()};
}

template <typename V, typename E>
YACLIB_INLINE auto operator co_await(Task<V, E>&& task) noexcept {
  YACLIB_ASSERT(task.Valid());
  return detail::TransferSingleAwaiter{std::move(task.GetCore())};
}

}  // namespace yaclib
