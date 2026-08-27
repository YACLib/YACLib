#pragma once

#include <yaclib/async/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/util/detail/mutex_event.hpp>

#include <chrono>
#include <cstddef>

namespace yaclib {

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param fs futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Clock, typename Duration, typename... Waited>
YACLIB_INLINE std::enable_if_t<(... && is_waitable_with_timeout_v<Waited>), bool> WaitUntil(
  const std::chrono::time_point<Clock, Duration>& timeout_time, Waited&... fs) noexcept {
  YACLIB_ASSERT(... && fs.Valid());
  return detail::WaitCore<Event>(timeout_time, fs.GetHandle()...);
}

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param begin iterator to futures to wait
 * \param end iterator to futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Clock, typename Duration, typename It, typename Sentinel,
          typename = std::enable_if_t<is_input_range_pair_v<It, Sentinel> &&
                                      is_waitable_with_timeout_v<typename std::iterator_traits<It>::reference>>>
YACLIB_INLINE bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, It begin,
                             Sentinel end) noexcept {
  static_assert(
    has_constant_time_distance_v<It, Sentinel>,
    "Use WaitUntil(timeout, begin, std::distance(begin, end)) instead");  // We don't use std::distance because we want
                                                                          // to alert the user to the fact that it can
                                                                          // be expensive.

  return WaitUntil<Event, Clock, Duration>(timeout_time, begin, static_cast<std::size_t>(end - begin));
}

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param range of futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Clock, typename Duration, typename Range,
          typename = std::enable_if_t<
            is_input_range_v<Range> &&
            is_waitable_with_timeout_v<typename std::iterator_traits<detail::RangeIterator<Range>>::reference>>>
YACLIB_INLINE bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Range&& range) noexcept {
  return WaitUntil<Event, Clock, Duration>(timeout_time, std::begin(range), std::end(range));
}

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param begin iterator to futures to wait
 * \param count of futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Clock, typename Duration, typename It,
          typename = std::enable_if_t<is_input_iterator_v<It> &&
                                      is_waitable_with_timeout_v<typename std::iterator_traits<It>::reference>>>
YACLIB_INLINE bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, It begin,
                             std::size_t count) noexcept {
  return detail::WaitIterator<Event>(timeout_time, begin, count);
}

}  // namespace yaclib
