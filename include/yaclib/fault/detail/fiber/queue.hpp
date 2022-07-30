#pragma once

#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/fault/detail/fiber/system_clock.hpp>
#include <yaclib/fault/detail/wait_status.hpp>

#include <vector>

namespace yaclib::detail::fiber {

struct NoTimeoutTag final {};

class FiberQueue final {
 public:
  FiberQueue() noexcept = default;
  FiberQueue(FiberQueue&& other) = default;
  FiberQueue& operator=(FiberQueue&& other) noexcept;

  WaitStatus Wait(NoTimeoutTag);

  template <typename Rep, typename Period>
  WaitStatus Wait(const std::chrono::duration<Rep, Period>& duration) {
    return Wait(duration + SystemClock::now());
  }

  template <typename Clock, typename Duration>
  WaitStatus Wait(const std::chrono::time_point<Clock, Duration>& time_point) {
    auto* fiber = fault::Scheduler::Current();
    auto* queue_node = static_cast<BiNodeWaitQueue*>(fiber);
    _queue.PushBack(queue_node);
    auto* scheduler = fault::Scheduler::GetScheduler();
    scheduler->SleepPreemptive(
      std::chrono::duration_cast<std::chrono::nanoseconds>(time_point.time_since_epoch()).count());
    bool res = queue_node->Erase();
    return res ? WaitStatus::Timeout : WaitStatus::Ready;
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
