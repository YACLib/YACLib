#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/await_inline.hpp>
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
  return AwaitInline(waited);
}

template <typename... Waited, typename = std::enable_if_t<(... && is_waitable_v<Waited>)>>
YACLIB_INLINE auto Await(Waited&... waited) noexcept {
  return AwaitInline(waited...);
}

template <typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type,
          typename = std::enable_if_t<is_waitable_v<Value>>>
YACLIB_INLINE auto Await(Iterator begin, std::size_t count) noexcept {
  return AwaitInline(begin, count);
}

template <typename Iterator,
          typename = std::enable_if_t<is_waitable_v<typename std::iterator_traits<Iterator>::value_type>>>
YACLIB_INLINE auto Await(Iterator begin, Iterator end) noexcept {
  return AwaitInline(begin, end);
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
