#pragma once

#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/fault/detail/fiber/system_clock.hpp>

#include <vector>

namespace yaclib::detail::fiber {

struct NoTimeoutTag {};

class FiberQueue {
 public:
  FiberQueue() noexcept = default;
  FiberQueue(FiberQueue&& other) = default;
  FiberQueue& operator=(FiberQueue&& other) noexcept;

  bool Wait(NoTimeoutTag);

  template <typename Rep, typename Period>
  bool Wait(const std::chrono::duration<Rep, Period>& duration) {
    return Wait(duration + SystemClock::now());
  }

  template <typename Clock, typename Duration>
  bool Wait(const std::chrono::time_point<Clock, Duration>& time_point) {
    auto* fiber = fault::Scheduler::Current();
    auto* queue_node = static_cast<BiNodeWaitQueue*>(fiber);
    _queue.PushBack(queue_node);
    auto* scheduler = fault::Scheduler::GetScheduler();
    scheduler->SleepPreemptive(
      std::chrono::duration_cast<std::chrono::nanoseconds>(time_point.time_since_epoch()).count());
    bool res = queue_node->Erase();
    return res;
  }

  void NotifyAll();

  void NotifyOne();

  [[nodiscard]] bool Empty() const noexcept;

  ~FiberQueue();

 private:
  static void ScheduleAndRemove(FiberBase* node);

  BiList _queue;
};

}  // namespace yaclib::detail::fiber
