#pragma once

#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/fiber.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/log.hpp>

#include <chrono>
#include <list>
#include <map>
#include <random>
#include <vector>

namespace yaclib::detail {

class Scheduler {
 public:
  friend class FiberQueue;

  Scheduler() : _running(false), _time(0), _rand(228) {
  }

  [[nodiscard]] bool IsRunning() const;

  static void Suspend();

  template <class Clock, class Duration>
  auto SleepUntil(const std::chrono::time_point<Clock, Duration>& sleep_time) {
    std::chrono::time_point<Clock, Duration> now = Clock::now();
    Duration duration = sleep_time - now;
    return SleepFor(duration);
  }

  template <class Rep, class Period>
  auto SleepFor(const std::chrono::duration<Rep, Period>& sleep_duration) {
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(sleep_duration).count();
    if (us <= 0) {
      return std::pair{us, std::list<IntrusivePtr<Fiber>>::iterator()};
    }
    auto time = GetTimeUs() + us;
    auto current_fiber = Current();
    std::list<IntrusivePtr<Fiber>>& sleep_list = _sleep_list[GetTimeUs() + us];
    sleep_list.push_back(current_fiber);
    auto iter = sleep_list.end();
    iter--;
    Suspend();
    return std::pair{time, iter};
  }

  [[nodiscard]] uint64_t GetTimeUs() const;

  static IntrusivePtr<Fiber> Current();

  static Fiber::Id GetId();

  [[nodiscard]] uint64_t GetRandNumber();

  static void RescheduleCurrent();

  void Run(const IntrusivePtr<Fiber>& fiber);

 private:
  void AdvanceTime();

  void TickTime();

  IntrusivePtr<Fiber> GetNext();

  void RunLoop();

  void WakeUpNeeded();
  std::mt19937 _rand;
  uint64_t _time;
  std::vector<IntrusivePtr<Fiber>> _queue;
  std::map<uint64_t, std::list<IntrusivePtr<Fiber>>> _sleep_list;
  bool _running;
};

Scheduler* GetScheduler();

IntrusivePtr<Fiber> PollRandomElementFromList(std::vector<IntrusivePtr<Fiber>>& list);

}  // namespace yaclib::detail
