#pragma once

#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/log.hpp>

#include <chrono>
#include <map>

namespace yaclib::detail::fiber {

class FiberQueue;

}  // namespace yaclib::detail::fiber
namespace yaclib::fault {

class Scheduler final {
 public:
  Scheduler() noexcept = default;

  void Schedule(detail::fiber::FiberBase* fiber);

  [[nodiscard]] bool IsRunning() const noexcept;

  void Sleep(uint64_t ns);

  void SleepPreemptive(uint64_t ns);

  [[nodiscard]] uint64_t GetTimeNs() const noexcept;

  static detail::fiber::FiberBase* Current() noexcept;

  static detail::fiber::FiberBase::Id GetId();

  static void RescheduleCurrent();

  static void SetTickLength(uint32_t tick) noexcept;

  static void SetRandomListPick(uint32_t k) noexcept;

  void Stop();

  static Scheduler* GetScheduler() noexcept;

  static void Set(Scheduler* scheduler) noexcept;

  static void Suspend();

 private:
  void AdvanceTime() noexcept;

  void TickTime() noexcept;

  detail::fiber::FiberBase* GetNext();

  void RunLoop();

  void WakeUpNeeded() noexcept;

  // TODO(myannyax) priority queue?
  std::map<uint64_t, detail::fiber::BiList> _sleep_list;
  // TODO(myannyax) priority queue?
  detail::fiber::BiList _queue;
  uint64_t _time{0};
  bool _running{false};
};

}  // namespace yaclib::fault

namespace yaclib::detail::fiber {

Node* PollRandomElementFromList(BiList& list);

}  // namespace yaclib::detail::fiber
