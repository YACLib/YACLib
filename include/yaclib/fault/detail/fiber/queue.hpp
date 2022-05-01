#pragma once
#include <yaclib/fault/detail/fiber/fiber.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <vector>

namespace yaclib::detail {
class FiberQueue {
 public:
  void Wait();

  template <typename Rep, typename Period>
  bool Wait(const std::chrono::duration<Rep, Period>& duration) {
    auto* fiber = Scheduler::Current();
    auto* queue_node = dynamic_cast<BiNodeWaitQueue*>(fiber);
    _queue.PushBack(queue_node);
    auto* scheduler = GetScheduler();
    auto time = scheduler->SleepFor(duration);
    if (scheduler->_sleep_list.find(time) != scheduler->_sleep_list.end()) {
      scheduler->_sleep_list[time].Erase(dynamic_cast<BiNodeSleep*>(fiber));
      if (scheduler->_sleep_list[time].Empty()) {
        scheduler->_sleep_list.erase(time);
      }
    }
    bool res = _queue.Erase(queue_node);
    return res;
  }

  template <typename Clock, typename Duration>
  bool Wait(const std::chrono::time_point<Clock, Duration>& time_point) {
    auto* fiber = Scheduler::Current();
    auto* queue_node = dynamic_cast<BiNodeWaitQueue*>(fiber);
    _queue.PushBack(queue_node);
    auto* scheduler = GetScheduler();
    auto time = scheduler->SleepUntil(time_point);
    if (scheduler->_sleep_list.find(time) != scheduler->_sleep_list.end()) {
      scheduler->_sleep_list[time].Erase(dynamic_cast<BiNodeSleep*>(fiber));
      if (scheduler->_sleep_list[time].Empty()) {
        scheduler->_sleep_list.erase(time);
      }
    }
    bool res = _queue.Erase(queue_node);
    return res;
  }

  void NotifyAll();

  void NotifyOne();

 private:
  BiList _queue;
};
}  // namespace yaclib::detail
