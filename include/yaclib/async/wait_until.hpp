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
  return detail::WaitCore<Event>(timeout_time, fs.GetBaseHandle()...);
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
template <typename Event = detail::MutexEvent, typename Clock, typename Duration, typename Iterator>
YACLIB_INLINE std::enable_if_t<is_waitable_with_timeout_v<typename std::iterator_traits<Iterator>::reference>, bool>
WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Iterator begin, Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Wait*(..., begin, distance(begin, end))
  return detail::WaitIterator<Event>(timeout_time, begin, static_cast<std::size_t>(end - begin));
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
template <typename Event = detail::MutexEvent, typename Clock, typename Duration, typename Iterator>
YACLIB_INLINE std::enable_if_t<is_waitable_with_timeout_v<typename std::iterator_traits<Iterator>::reference>, bool>
WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Iterator begin, std::size_t count) noexcept {
  return detail::WaitIterator<Event>(timeout_time, begin, count);
}

}  // namespace yaclib
