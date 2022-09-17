#pragma once

#include <yaclib/util/ref.hpp>

#include <chrono>
#include <yaclib_std/condition_variable>
#include <yaclib_std/mutex>

namespace yaclib::detail {

class MutexEvent {
 public:
  using Token = std::unique_lock<yaclib_std::mutex>;

  Token Make() noexcept;

  void Wait(Token& token) noexcept;

  template <typename Rep, typename Period>
  bool Wait(Token& token, const std::chrono::duration<Rep, Period>& timeout_duration) noexcept {
    return _cv.wait_for(token, timeout_duration, [&] {
      return _is_ready;
    });
  }

  template <typename Clock, typename Duration>
  bool Wait(Token& token, const std::chrono::time_point<Clock, Duration>& timeout_time) noexcept {
    return _cv.wait_until(token, timeout_time, [&] {
      return _is_ready;
    });
  }

  void Set() noexcept;

  void Reset() noexcept;

 private:
  bool _is_ready = false;
  yaclib_std::mutex _m;
  yaclib_std::condition_variable _cv;
};

}  // namespace yaclib::detail
