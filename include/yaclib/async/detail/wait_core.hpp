#pragma once

#include <yaclib/fault/condition_variable.hpp>
#include <yaclib/fault/mutex.hpp>
#include <yaclib/util/ref.hpp>

#include <chrono>

namespace yaclib::detail {

class WaitCore : public IRef {
 public:
  bool is_ready = false;
  yaclib_std::mutex m;
  yaclib_std::condition_variable cv;

  void Wait(std::unique_lock<yaclib_std::mutex>& lock);

  template <typename Rep, typename Period>
  bool Wait(std::unique_lock<yaclib_std::mutex>& lock, const std::chrono::duration<Rep, Period>& timeout_duration) {
    return cv.wait_for(lock, timeout_duration, [&] {
      return is_ready;
    });
  }

  template <typename Clock, typename Duration>
  bool Wait(std::unique_lock<yaclib_std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return cv.wait_until(lock, timeout_time, [&] {
      return is_ready;
    });
  }
};

struct WaitCoreDeleter {
  static void Delete(WaitCore* self) {
    std::lock_guard lock{self->m};
    self->is_ready = true;
    self->cv.notify_all();  // Notify under mutex, because cv located on stack memory of other thread
  }
};

}  // namespace yaclib::detail
