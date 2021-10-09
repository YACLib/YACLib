#pragma once

#include <yaclib/async/detail/core.hpp>
#include <yaclib/util/counters.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib::algo::detail {

enum class WaitPolicy {
  Endless,
  For,
  Until,
};

template <WaitPolicy kPolicy, typename Time, typename... Cores>
bool Wait(const Time& time, Cores&... cores) {
  static_assert(sizeof...(cores) > 0, "Number of futures must be more than zero");
  static_assert((... && std::is_same_v<async::detail::BaseCore, Cores>), "Fs must be futures in Wait function");
  util::Counter<async::detail::WaitCore, async::detail::WaitCoreDeleter> callback;
  callback.IncRef();  // Optimization: we don't want to notify when return true immediately
  if ((... & cores.SetWaitCallback(callback))) {
    return true;
  }
  callback.DecRef();
  bool ready{true};
  std::unique_lock guard{callback.m};
  if constexpr (kPolicy != WaitPolicy::Endless) {
    // If you have problem with TSAN here, check this link: https://github.com/google/sanitizers/issues/1259
    // TLDR: new pthread function is not supported by thread sanitizer yet.
    ready = [&] {
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
    guard.unlock();
    if ((... & cores.ResetAfterTimeout())) {
      return false;
    }
    // We know we have Result, but we must wait until callback was not used by executor
    guard.lock();
  }
  while (!callback.is_ready) {
    callback.cv.wait(guard);
  }
  return ready;
}

}  // namespace yaclib::algo::detail
