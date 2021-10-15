#pragma once

#include <yaclib/algo/detail/wait.hpp>

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
template <typename Clock, typename Duration, typename... Futures>
bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Futures&&... fs) {
  return detail::Wait<detail::WaitPolicy::Until>(timeout_time, static_cast<detail::BaseCore&>(*fs._core)...);
}

}  // namespace yaclib
