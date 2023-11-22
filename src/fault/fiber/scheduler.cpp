#include <fault/util.hpp>

#include <yaclib/fault/config.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <cstdio>

namespace yaclib::fault {

static Scheduler* sCurrentScheduler = nullptr;
static thread_local detail::fiber::FiberBase* sCurrent = nullptr;
static std::uint32_t sTickLength = 10;

detail::fiber::FiberBase* Scheduler::GetNext() {
  YACLIB_DEBUG(_queue.Empty(), "Queue can't be empty");
  auto* next = PollRandomElementFromList(_queue);
  return static_cast<detail::fiber::FiberBase*>(static_cast<detail::fiber::BiNodeScheduler*>(next));
}

bool Scheduler::IsRunning() const noexcept {
  return _running;
}

void Scheduler::Suspend() {
  auto* fiber = sCurrent;
  YACLIB_ASSERT(fiber != nullptr);
  fiber->Suspend();
}

Scheduler* Scheduler::GetScheduler() noexcept {
  return sCurrentScheduler;
}

void Scheduler::Set(Scheduler* scheduler) noexcept {
  sCurrentScheduler = scheduler;
}

void Scheduler::Schedule(detail::fiber::FiberBase* fiber) {
  fiber->SetState(detail::fiber::Waiting);
  _queue.PushBack(static_cast<detail::fiber::BiNodeScheduler*>(fiber));
  if (!_running) {
    _running = true;
    RunLoop();
    _running = false;
  }
}

detail::fiber::FiberBase* Scheduler::Current() noexcept {
  return sCurrent;
}

detail::fiber::FiberBase::Id Scheduler::GetId() {
  return sCurrent != nullptr ? sCurrent->GetId() : detail::fiber::FiberBase::Id{static_cast<std::uint64_t>(-1)};
}

void Scheduler::TickTime() noexcept {
  _time += sTickLength;
}

void Scheduler::AdvanceTime() noexcept {
  if (_sleep_list.begin()->first >= _time) {
    auto min_sleep_time = _sleep_list.begin()->first - _time;
    _time += min_sleep_time;
  }
}

std::uint64_t Scheduler::GetTimeNs() const noexcept {
  return _time;
}

void Scheduler::WakeUpNeeded() noexcept {
  auto iter_to_remove = _sleep_list.end();
  for (auto it = _sleep_list.begin(); it != _sleep_list.end(); it++) {
    if (it->first > _time) {
      iter_to_remove = it;
      break;
    }
    _queue.PushAll(std::move(it->second));
  }
  if (iter_to_remove != _sleep_list.begin()) {
    _sleep_list.erase(_sleep_list.begin(), iter_to_remove);
  }
}

void Scheduler::RunLoop() {
  while (!_queue.Empty() || !_sleep_list.empty()) {
    if (_queue.Empty()) {
      AdvanceTime();
    }
    WakeUpNeeded();
    auto* next = GetNext();
    sCurrent = next;
    TickTime();
    next->Resume();
    if (next->GetState() == detail::fiber::Completed && !next->IsThreadAlive()) {
      delete next;
    }
  }
  sCurrent = nullptr;
}

void Scheduler::RescheduleCurrent() {
  if (sCurrent == nullptr) {
    return;
  }
  auto* fiber = sCurrent;
  GetScheduler()->_queue.PushBack(static_cast<detail::fiber::BiNodeScheduler*>(fiber));
  fiber->Suspend();
}

void Scheduler::SetTickLength(std::uint32_t tick) noexcept {
  sTickLength = tick;
}

void Scheduler::Sleep(std::uint64_t ns) {
  if (ns <= GetTimeNs()) {
    return;
  }
  detail::fiber::BiList& sleep_list = _sleep_list[ns];
  auto* fiber = sCurrent;
  sleep_list.PushBack(static_cast<detail::fiber::BiNodeScheduler*>(fiber));
  Suspend();
}

void Scheduler::SleepPreemptive(std::uint64_t ns) {
  ns += detail::GetRandNumber(GetFaultSleepTime());
  Sleep(ns);
  // <= because wakeup called before time adjustment
  if (_time <= ns) {
    auto it = _sleep_list.find(ns);
    YACLIB_DEBUG(it == _sleep_list.end(), "sleep_list for time that is not passed yet isn't found");
    if (it->second.Empty()) {
      _sleep_list.erase(ns);
    }
  }
}

}  // namespace yaclib::fault
namespace yaclib::detail::fiber {

static std::uint32_t sRandomListPick = 10;

void SetRandomListPick(std::uint32_t k) noexcept {
  sRandomListPick = k;
}

Node* PollRandomElementFromList(BiList& list) {
  auto rand_pos = detail::GetRandNumber(2 * sRandomListPick);
  auto reversed = false;
  if (rand_pos >= sRandomListPick) {
    reversed = true;
    rand_pos -= sRandomListPick;
  }
  auto* next = list.GetElement(rand_pos, reversed);
  next->Erase();
  return next;
}

}  // namespace yaclib::detail::fiber
