#pragma once

#include <yaclib/fault/detail/fiber/coroutine_base.hpp>
#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/fiber.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/log.hpp>

#include <chrono>
#include <map>
#include <random>
#include <vector>

namespace yaclib::detail {

static thread_local IntrusivePtr<Fiber> current;

class Scheduler {
 public:
  friend class FiberThreadlike;
  friend class FiberQueue;

  Scheduler() : _sleep_list(), _running(false), _time(0), _rand(228) {
  }

  void Run(Routine routine);

  [[nodiscard]] bool IsRunning() const;

  static void Suspend();

  template <class Clock, class Duration>
  void SleepUntil(const std::chrono::time_point<Clock, Duration>& sleep_time) {
    std::chrono::time_point<Clock, Duration> now = Clock::now();
    Duration duration = sleep_time - now;
    SleepFor(duration);
  }

  template <class Rep, class Period>
  void SleepFor(const std::chrono::duration<Rep, Period>& sleep_duration) {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(sleep_duration).count();
    if (us <= 0) {
      return;
    }
    auto currentFiber = Current();
    _sleep_list[GetTimeUs() + us].push_back(currentFiber);
    Suspend();
  }

  [[nodiscard]] unsigned long GetTimeUs() const;

  static IntrusivePtr<Fiber> Current();

  static Fiber::Id GetId();

  unsigned long GetRandNumber();

  static void RescheduleCurrent();

  void Unschedule(IntrusivePtr<Fiber> fiber);

 private:
  void Run(IntrusivePtr<Fiber> fiber);

  void AdvanceTime();

  void TickTime();

  IntrusivePtr<Fiber> GetNext();

  void RunLoop();

  void WakeUpNeeded();
  std::mt19937 _rand;
  unsigned long _time;
  std::vector<IntrusivePtr<Fiber>> _queue;
  std::map<long, std::vector<IntrusivePtr<Fiber>>> _sleep_list;
  bool _running;
};

Scheduler* GetScheduler();

IntrusivePtr<Fiber> PollRandomElementFromList(std::vector<IntrusivePtr<Fiber>>& list);

}  // namespace yaclib::detail
