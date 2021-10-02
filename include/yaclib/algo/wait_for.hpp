#pragma once

#include <yaclib/algo/detail/wait_impl.hpp>

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
template <typename Rep, typename Period, typename... T>
bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, Future<T>&... fs) {
  return detail::Wait(timeout_duration, static_cast<detail::BaseCore&>(*fs.GetCore())...);
}

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param begin Iterator to futures to wait
 * \param end Iterator to futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Rep, typename Period, typename Iterator>
std::enable_if_t<!util::IsFutureV<Iterator>, bool> WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration,
                                                           Iterator begin, Iterator end) {
  return detail::Wait(timeout_duration, begin, begin, end);
}

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param begin Iterator to futures to wait
 * \param size count of futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Rep, typename Period, typename Iterator>
bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, Iterator begin, size_t size) {
  return detail::Wait(timeout_duration, begin, 0, size);
}

}  // namespace yaclib
