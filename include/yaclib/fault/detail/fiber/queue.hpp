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
    auto* scheduler = GetScheduler();
    auto iter = scheduler->SleepFor(duration);
    if (scheduler->_sleep_list.find(iter.first) != scheduler->_sleep_list.end() &&
        iter.second != scheduler->_sleep_list[iter.first].end()) {
      scheduler->_sleep_list[iter.first].erase(iter.second);
      if (scheduler->_sleep_list[iter.first].empty()) {
        scheduler->_sleep_list.erase(iter.first);
      }
    }
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
    auto* scheduler = GetScheduler();
    auto iter = scheduler->template SleepUntil(time_point);
    if (scheduler->_sleep_list.find(iter.first) != scheduler->_sleep_list.end() &&
        iter.second != scheduler->_sleep_list[iter.first].end()) {
      scheduler->_sleep_list[iter.first].erase(iter.second);
      if (scheduler->_sleep_list[iter.first].empty()) {
        scheduler->_sleep_list.erase(iter.first);
      }
    }
    _queue.erase(std::remove_if(_queue.begin(), _queue.end(),
                                [&](const auto& item) {
                                  return item == fiber;
                                }),
                 _queue.end());
  }

  void NotifyAll();

  void NotifyOne();

  bool IsEmpty();

 private:
  std::vector<IntrusivePtr<Fiber>> _queue;
};
}  // namespace yaclib::detail
