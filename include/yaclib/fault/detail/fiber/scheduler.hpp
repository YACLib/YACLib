#pragma once

#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/func.hpp>

#include <chrono>
#include <map>

namespace yaclib::detail::fiber {
class FiberQueue;
}  // namespace yaclib::detail::fiber

namespace yaclib::fault {

class Scheduler {
 public:
  Scheduler();

  void Schedule(detail::fiber::FiberBase* fiber);

  [[nodiscard]] bool IsRunning() const;

  void Sleep(uint64_t ns);

  [[nodiscard]] uint64_t GetTimeNs() const;

  static detail::fiber::FiberBase* Current();

  static detail::fiber::FiberBase::Id GetId();

  static void RescheduleCurrent();

  static void SetTickLength(uint32_t tick);

  static void SetRandomListPick(uint32_t k);

  void Stop();

  static Scheduler* GetScheduler();

  static void Set(Scheduler* scheduler);

  static void Suspend();

 private:
  void AdvanceTime();

  void TickTime();

  detail::fiber::FiberBase* GetNext();

  void RunLoop();

  void WakeUpNeeded();

  uint64_t _time;
  detail::fiber::BiList _queue;
  std::map<uint64_t, detail::fiber::BiList> _sleep_list;
  bool _running;
};

}  // namespace yaclib::fault

namespace yaclib::detail::fiber {
BiNode* PollRandomElementFromList(BiList& list);
}  // namespace yaclib::detail::fiber
