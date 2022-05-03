#pragma once
#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/fault/detail/fiber/steady_clock.hpp>

#include <vector>

namespace yaclib::detail::fiber {

struct NoTimeoutTag {};

class FiberQueue {
 public:
  FiberQueue() = default;
  FiberQueue(FiberQueue&& other) = default;
  FiberQueue& operator=(FiberQueue&& other) noexcept;

  bool Wait(NoTimeoutTag);

  template <typename Rep, typename Period>
  bool Wait(const std::chrono::duration<Rep, Period>& duration) {
    return Wait(duration + SteadyClock::now());
  }

  template <typename Clock, typename Duration>
  bool Wait(const std::chrono::time_point<Clock, Duration>& time_point) {
    auto* fiber = fault::Scheduler::Current();
    auto* queue_node = static_cast<BiNodeWaitQueue*>(fiber);
    _queue.PushBack(queue_node);
    auto* scheduler = fault::Scheduler::GetScheduler();
    scheduler->Sleep(time_point.time_since_epoch().count());
    bool res = _queue.Erase(queue_node);
    return res;
  }

  void NotifyAll();

  void NotifyOne();

  bool Empty() const;

  ~FiberQueue();

 private:
  BiList _queue;
};
}  // namespace yaclib::detail::fiber
