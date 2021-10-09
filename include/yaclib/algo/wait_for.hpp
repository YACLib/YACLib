#pragma once

#include <yaclib/algo/detail/wait.hpp>

namespace yaclib::algo {

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param fs futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Rep, typename Period, typename... Futures>
bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, Futures&&... fs) {
  return detail::Wait<detail::WaitPolicy::For>(timeout_duration, static_cast<async::detail::BaseCore&>(*fs._core)...);
}

}  // namespace yaclib::algo
