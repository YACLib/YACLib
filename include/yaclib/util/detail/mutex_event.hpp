#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/condition_variable.hpp>
#include <yaclib/fault/mutex.hpp>
#include <yaclib/util/ref.hpp>

#include <chrono>

namespace yaclib::detail {

class /*alignas(kCacheLineSize)*/ MutexEvent : public IRef {
 public:
  using Token = std::unique_lock<yaclib_std::mutex>;

  Token Make();

  void Wait(Token& token);

  template <typename Rep, typename Period>
  bool Wait(Token& token, const std::chrono::duration<Rep, Period>& timeout_duration) {
    return _cv.wait_for(token, timeout_duration, [&] {
      return _is_ready;
    });
  }

  template <typename Clock, typename Duration>
  bool Wait(Token& token, const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return _cv.wait_until(token, timeout_time, [&] {
      return _is_ready;
    });
  }

  void Reset(Token&) noexcept;

  void Set();

 private:
  bool _is_ready = false;
  yaclib_std::mutex _m;
  yaclib_std::condition_variable _cv;
};

}  // namespace yaclib::detail
