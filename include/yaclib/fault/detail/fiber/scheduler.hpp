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

void SetRandomListPick(std::uint32_t k) noexcept;

Node* PollRandomElementFromList(BiList& list);

}  // namespace yaclib::detail::fiber
namespace yaclib::fault {

class Scheduler final {
 public:
  static void SetTickLength(std::uint32_t tick) noexcept;

  [[nodiscard]] bool IsRunning() const noexcept;

  [[nodiscard]] std::uint64_t GetTimeNs() const noexcept;

  void Schedule(detail::fiber::FiberBase* fiber);

  static void RescheduleCurrent();

  void Sleep(std::uint64_t ns);

  void SleepPreemptive(std::uint64_t ns);

  static detail::fiber::FiberBase* Current() noexcept;

  static detail::fiber::FiberBase::Id GetId();

  static Scheduler* GetScheduler() noexcept;

  static void Set(Scheduler* scheduler) noexcept;

  static void Suspend();

 private:
  void AdvanceTime() noexcept;

  void TickTime() noexcept;

  detail::fiber::FiberBase* GetNext();

  void RunLoop();

  void WakeUpNeeded() noexcept;

  std::uint32_t _random_list_pick = 10;

  // TODO(myannyax) priority queue?
  std::map<std::uint64_t, detail::fiber::BiList> _sleep_list;
  // TODO(myannyax) priority queue?
  detail::fiber::BiList _queue;
  std::uint64_t _time{0};
  bool _running{false};
};

}  // namespace yaclib::fault
