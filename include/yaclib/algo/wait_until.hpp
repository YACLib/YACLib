#pragma once

#include <yaclib/algo/detail/wait_impl.hpp>
#include <yaclib/async/future.hpp>

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
template <typename Clock, typename Duration, typename... V, typename... E>
bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Future<V, E>&... fs) {
  return detail::WaitCores(timeout_time, static_cast<detail::BaseCore&>(*fs.GetCore())...);
}

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param begin Iterator to futures to wait
 * \param end Iterator to futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Clock, typename Duration, typename Iterator>
std::enable_if_t<!is_future_v<Iterator>, bool> WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time,
                                                         Iterator begin, Iterator end) {
  return detail::WaitIters(timeout_time, begin, begin, end);
}

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param begin Iterator to futures to wait
 * \param size count of futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Clock, typename Duration, typename Iterator>
bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Iterator begin, std::size_t size) {
  return detail::WaitIters(timeout_time, begin, std::size_t{0}, size);
}

}  // namespace yaclib
