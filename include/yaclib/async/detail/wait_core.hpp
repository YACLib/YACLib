#pragma once

#include <yaclib/util/ref.hpp>

#include <condition_variable>
#include <mutex>

namespace yaclib::detail {

class WaitCore : public util::IRef {
 public:
  bool is_ready{false};
  std::mutex m;
  std::condition_variable cv;

  void Wait(std::unique_lock<std::mutex>& guard);

  template <typename Rep, typename Period>
  bool Wait(std::unique_lock<std::mutex>& guard, const std::chrono::duration<Rep, Period>& timeout_duration) {
    return cv.wait_for(guard, timeout_duration, [&] {
      return is_ready;
    });
  }

  template <typename Clock, typename Duration>
  bool Wait(std::unique_lock<std::mutex>& guard, const std::chrono::time_point<Clock, Duration>& timeout_time) {
    return cv.wait_until(guard, timeout_time, [&] {
      return is_ready;
    });
  }
};

struct WaitCoreDeleter {
  static void Delete(WaitCore* self) {
    std::lock_guard guard{self->m};
    self->is_ready = true;
    self->cv.notify_all();  // Notify under mutex, because cv located on stack memory of other thread
  }
};

}  // namespace yaclib::detail
