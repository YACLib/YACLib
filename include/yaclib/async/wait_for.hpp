#pragma once

#include <yaclib/async/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/detail/mutex_event.hpp>

#include <chrono>
#include <cstddef>

namespace yaclib {

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param fs futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename... Waited>
YACLIB_INLINE std::enable_if_t<(... && is_waitable_with_timeout_v<Waited>), bool> WaitFor(
  const std::chrono::duration<Rep, Period>& timeout_duration, Waited&... fs) noexcept {
  YACLIB_ASSERT(... && fs.Valid());
  return detail::WaitCore<Event>(timeout_duration, fs.GetHandle()...);
}

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param begin iterator to futures to wait
 * \param end iterator to futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename It, typename Sentinel,
          typename = std::enable_if_t<is_input_range_pair_v<It, Sentinel> &&
                                      is_waitable_with_timeout_v<typename std::iterator_traits<It>::reference>>>
YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, It begin,
                           Sentinel end) noexcept {
  static_assert(
    has_constant_time_distance_v<It, Sentinel>,
    "Use WaitFor(timeout, begin, std::distance(begin, end)) instead");  // We don't use std::distance because we want to
                                                                        // alert the user to the fact that it can be
                                                                        // expensive.

  return WaitFor<Event, Rep, Period>(timeout_duration, begin, static_cast<std::size_t>(end - begin));
}

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param range of futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename Range,
          typename = std::enable_if_t<
            is_input_range_v<Range> &&
            is_waitable_with_timeout_v<typename std::iterator_traits<detail::RangeIterator<Range>>::reference>>>
YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, Range&& range) noexcept {
  return WaitFor<Event, Rep, Period>(timeout_duration, std::begin(range), std::end(range));
}

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param begin iterator to futures to wait
 * \param count of futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename It,
          typename = std::enable_if_t<is_input_iterator_v<It> &&
                                      is_waitable_with_timeout_v<typename std::iterator_traits<It>::reference>>>
YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, It begin,
                           std::size_t count) noexcept {
  return detail::WaitIterator<Event>(timeout_duration, begin, count);
}

}  // namespace yaclib
