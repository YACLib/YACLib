#pragma once

#include <yaclib/async/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/default_event.hpp>

#include <cstddef>

namespace yaclib {

/**
 * Wait until \ref Ready becomes true
 *
 * \param fs one or more futures to wait
 */
template <typename Event = detail::DefaultEvent, typename... Waited>
YACLIB_INLINE std::enable_if_t<(... && is_waitable_v<Waited>), void> Wait(Waited&... fs) noexcept {
  YACLIB_ASSERT(... && fs.Valid());
  detail::WaitCore<Event>(detail::NoTimeoutTag{}, fs.GetHandle()...);
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin iterator to futures to wait
 * \param end iterator to futures to wait
 */
template <typename Event = detail::DefaultEvent, typename It, typename Sentinel,
          typename = std::enable_if_t<is_input_range_pair_v<It, Sentinel> &&
                                      is_waitable_v<typename std::iterator_traits<It>::reference>>>
YACLIB_INLINE void Wait(It begin, Sentinel end) noexcept {
  static_assert(
    has_constant_time_distance_v<It, Sentinel>,
    "Use Wait(begin, std::distance(begin, end)) instead");  // We don't use std::distance because we want to alert
                                                            // the user to the fact that it can be expensive.

  Wait<Event>(begin, static_cast<std::size_t>(end - begin));
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param range of futures to wait
 */
template <
  typename Event = detail::DefaultEvent, typename Range,
  typename = std::enable_if_t<is_input_range_v<Range> &&
                              is_waitable_v<typename std::iterator_traits<detail::RangeIterator<Range>>::reference>>>
YACLIB_INLINE void Wait(Range&& range) noexcept {
  Wait<Event>(std::begin(range), std::end(range));
}

/**
 * Wait until \ref Ready becomes true
 *
 * \param begin iterator to futures to wait
 * \param count of futures to wait
 */
template <
  typename Event = detail::DefaultEvent, typename It,
  typename = std::enable_if_t<is_input_iterator_v<It> && is_waitable_v<typename std::iterator_traits<It>::reference>>>
YACLIB_INLINE void Wait(It begin, std::size_t count) noexcept {
  detail::WaitIterator<Event>(detail::NoTimeoutTag{}, begin, count);
}

}  // namespace yaclib
