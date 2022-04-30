#pragma once
#include <yaclib/fault/detail/fiber/fiber.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <vector>

namespace yaclib::detail {
class FiberQueue {
 public:
  void Wait();

  template <typename Rep, typename Period>
  void Wait(const std::chrono::duration<Rep, Period>& duration) {
    auto fiber = Scheduler::Current();
    _queue.push_back(fiber);
    GetScheduler()->SleepFor(duration);
    GetScheduler()->Unschedule(fiber);
    _queue.erase(std::remove_if(_queue.begin(), _queue.end(),
                                [&](const auto& item) {
                                  return item == fiber;
                                }),
                 _queue.end());
  }

  template <typename Clock, typename Duration>
  void Wait(const std::chrono::time_point<Clock, Duration>& time_point) {
    auto fiber = Scheduler::Current();
    _queue.push_back(fiber);
    GetScheduler()->SleepUntil(time_point);
    GetScheduler()->Unschedule(fiber);
    _queue.erase(std::remove_if(_queue.begin(), _queue.end(),
                                [&](const auto& item) {
                                  return item == fiber;
                                }),
                 _queue.end());
  }

  void NotifyAll();

  void NotifyOne();

 private:
  std::vector<IntrusivePtr<Fiber>> _queue;
};
}  // namespace yaclib::detail
