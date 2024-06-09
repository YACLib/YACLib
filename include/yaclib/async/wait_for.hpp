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
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename... V, typename... E>
YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration,
                           FutureBase<V, E>&... fs) noexcept {
  YACLIB_ASSERT(... && fs.Valid());
  return detail::WaitCore<Event>(timeout_duration, UpCast<detail::BaseCore>(*fs.GetCore())...);
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
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename Iterator>
YACLIB_INLINE std::enable_if_t<!is_future_base_v<Iterator>, bool> WaitFor(
  const std::chrono::duration<Rep, Period>& timeout_duration, Iterator begin, Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Wait*(..., begin, distance(begin, end))
  return detail::WaitIterator<Event>(timeout_duration, begin, static_cast<std::size_t>(end - begin));
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
template <typename Event = detail::MutexEvent, typename Rep, typename Period, typename Iterator>
YACLIB_INLINE bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, Iterator begin,
                           std::size_t count) noexcept {
  return detail::WaitIterator<Event>(timeout_duration, begin, count);
}

}  // namespace yaclib
