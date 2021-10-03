#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/util/counters.hpp>
#include <yaclib/util/type_traits.hpp>

#include <array>
#include <iostream>
#include <type_traits>

namespace yaclib::algo {
namespace detail {

enum class WaitPolicy {
  Endless,
  For,
  Until,
};

template <WaitPolicy kPolicy, typename Time, typename... Fs>
bool Wait(const Time& time, Fs&&... futures) {
  static_assert(sizeof...(futures) > 0, "Number of futures must be more than zero");
  static_assert((... && util::IsFutureV<std::decay_t<Fs>>), "Fs must be futures in Wait function");
  if ((... && futures.Ready())) {
    std::cout << "------------------------GGGGGGGGGGGGGGGG" << std::endl;
    return true;
  }

  util::Counter<async::detail::WaitCore, async::detail::WaitCoreDeleter> callback;
  /*if ((... & futures._core->SetWaitCallback(&callback))) {
    std::cout << "------------------------LLLLLLLLLLLLL" << std::endl;
    return true;
  }*/
  (..., futures._core->SetWaitCallback(&callback));

  std::unique_lock guard{callback.m};
  if constexpr (kPolicy != WaitPolicy::Endless) {
    const bool ready = [&] {
      if constexpr (kPolicy == WaitPolicy::For) {
        return callback.cv.wait_for(guard, time, [&] {
          return callback.is_ready;
        });
      } else if constexpr (kPolicy == WaitPolicy::Until) {
        return callback.cv.wait_until(guard, time, [&] {
          return callback.is_ready;
        });
      }
    }();

    if (ready) {
      return true;
    }
    if ((... | futures._core->ResetAfterTimeout())) {
      return false;
    }
    // We know we have Result, but we must wait until callback was not used by executor
  }

  while (!callback.is_ready) {
    std::cout << "------------------------AAAAAAAAAAAAAAAAAA" << std::endl;
    callback.cv.wait(guard);
  }
  std::cout << "------------------------fffffffffffffffffff" << std::endl;

  return true;
}

}  // namespace detail

/**
 * Wait until \ref Ready becomes true
 *
 * \param futures one or more futures to wait
 */
template <typename... Fs>
bool Wait(Fs&&... futures) {
  return detail::Wait<detail::WaitPolicy::Endless>(/* stub value */ false, futures...);
}

/**
 * Wait until specified time has been reached or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than until after timeout_time has been reached
 * due to scheduling or resource contention delays.
 * \param timeout_time maximum time point to block until
 * \param futures futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Clock, typename Duration, typename... Fs>
bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout_time, Fs&&... futures) {
  return detail::Wait<detail::WaitPolicy::Until>(timeout_time, std::forward<Fs>(futures)...);
}

/**
 * Wait until the specified timeout duration has elapsed or \ref Ready becomes true
 *
 * The behavior is undefined if \ref Valid is false before the call to this function.
 * This function may block for longer than timeout_duration due to scheduling or resource contention delays.
 * \param timeout_duration maximum duration to block for
 * \param futures futures to wait
 * \return The result of \ref Ready upon exiting
 */
template <typename Rep, typename Period, typename... Fs>
bool WaitFor(const std::chrono::duration<Rep, Period>& timeout_duration, Fs&&... futures) {
  return detail::Wait<detail::WaitPolicy::For>(timeout_duration, futures...);
}

}  // namespace yaclib::algo
