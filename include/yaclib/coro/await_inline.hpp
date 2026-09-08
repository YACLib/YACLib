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

template <typename It, typename = std::enable_if_t<is_input_iterator_v<It> &&
                                                   is_waitable_v<typename std::iterator_traits<It>::value_type>>>
YACLIB_INLINE auto AwaitInline(It begin, std::size_t count) noexcept {
  using namespace detail;
  using Value = typename std::iterator_traits<It>::value_type;
  static constexpr auto kShared = std::is_same_v<typename Value::Handle, SharedHandle>;
  using Awaiter = std::conditional_t<kShared, MultiAwaitAwaiter<DynamicSharedEvent<AwaitEvent<false>>>,
                                     MultiAwaitAwaiter<AwaitEvent<false>>>;
  return Awaiter{begin, count};
}

template <typename It, typename Sentinel,
          typename = std::enable_if_t<is_input_range_pair_v<It, Sentinel> &&
                                      is_waitable_v<typename std::iterator_traits<It>::value_type>>>
YACLIB_INLINE auto AwaitInline(It begin, Sentinel end) noexcept {
  static_assert(has_constant_time_distance_v<It, Sentinel>,
                "Use AwaitInline(begin, std::distance(begin, end)) instead");  // We don't use std::distance because we
                                                                               // want to alert the user to the fact
                                                                               // that it can be expensive.

  return AwaitInline(begin, static_cast<std::size_t>(end - begin));
}

template <typename Range, typename = std::enable_if_t<
                            is_input_range_v<Range> &&
                            is_waitable_v<typename std::iterator_traits<detail::RangeIterator<Range>>::value_type>>>
YACLIB_INLINE auto AwaitInline(Range&& range) noexcept {
  return AwaitInline(std::begin(range), std::end(range));
}

}  // namespace yaclib
