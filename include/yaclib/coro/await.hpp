#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/await_inline.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename V, typename T>
YACLIB_INLINE auto Await(Task<V, T>& task) noexcept {
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

template <typename It, typename = std::enable_if_t<is_input_iterator_v<It> &&
                                                   is_waitable_v<typename std::iterator_traits<It>::value_type>>>
YACLIB_INLINE auto Await(It begin, std::size_t count) noexcept {
  return AwaitInline(begin, count);
}

template <typename It, typename Sentinel,
          typename = std::enable_if_t<is_input_range_pair_v<It, Sentinel> &&
                                      is_waitable_v<typename std::iterator_traits<It>::value_type>>>
YACLIB_INLINE auto Await(It begin, Sentinel end) noexcept {
  static_assert(
    has_constant_time_distance_v<It, Sentinel>,
    "Use Await(begin, std::distance(begin, end)) instead");  // We don't use std::distance because we want to alert
                                                             // the user to the fact that it can be expensive.

  return Await(begin, static_cast<std::size_t>(end - begin));
}

template <typename Range, typename = std::enable_if_t<
                            is_input_range_v<Range> &&
                            is_waitable_v<typename std::iterator_traits<detail::RangeIterator<Range>>::value_type>>>
YACLIB_INLINE auto Await(Range&& range) noexcept {
  return Await(std::begin(range), std::end(range));
}

template <typename V, typename T>
YACLIB_INLINE auto operator co_await(FutureBase<V, T>&& future) noexcept {
  YACLIB_ASSERT(future.Valid());
  return detail::AwaitSingleAwaiter<false, V, T>{std::move(future.GetCore())};
}

template <typename V, typename T>
YACLIB_INLINE auto operator co_await(const SharedFutureBase<V, T>& future) noexcept {
  YACLIB_ASSERT(future.Valid());
  return detail::AwaitSingleAwaiter<true, V, T>{future.GetCore()};
}

template <typename V, typename T>
YACLIB_INLINE auto operator co_await(Task<V, T>&& task) noexcept {
  YACLIB_ASSERT(task.Valid());
  return detail::TransferSingleAwaiter{std::move(task.GetCore())};
}

}  // namespace yaclib
