#include <fault/util.hpp>

#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <iostream>

namespace yaclib::fault {

Scheduler* current_scheduler = nullptr;

static thread_local detail::fiber::FiberBase* current;

static uint32_t _tick_length = 10;
static uint32_t _random_list_pick = 0;

detail::fiber::FiberBase* Scheduler::GetNext() {
  YACLIB_DEBUG(_queue.Empty(), "Queue can't be empty");
  auto* next = PollRandomElementFromList(_queue);
  return static_cast<detail::fiber::FiberBase*>(static_cast<detail::fiber::BiNodeScheduler*>(next));
}

bool Scheduler::IsRunning() const {
  return _running;
}

void Scheduler::Suspend() {
  auto* fiber = current;
  fiber->Yield();
}

void Scheduler::Stop() {
  Scheduler::Set(nullptr);
  YACLIB_DEBUG(_running, "scheduler still running on stop");
}

Scheduler* Scheduler::GetScheduler() {
  return current_scheduler;
}

void Scheduler::Set(Scheduler* scheduler) {
  current_scheduler = scheduler;
}

void Scheduler::Schedule(detail::fiber::FiberBase* fiber) {
  InjectFault();
  _queue.PushBack(static_cast<detail::fiber::BiNodeScheduler*>(fiber));
  if (!IsRunning()) {
    _running = true;
    RunLoop();
    _running = false;
  }
}

detail::fiber::FiberBase* Scheduler::Current() {
  return current;
}

detail::fiber::FiberBase::Id Scheduler::GetId() {
  YACLIB_DEBUG(current == nullptr, "Current can't be null");
  return current->GetId();
}

void Scheduler::TickTime() {
  _time += _tick_length;
}

void Scheduler::AdvanceTime() {
  if (_sleep_list.begin()->first >= _time) {
    auto min_sleep_time = _sleep_list.begin()->first - _time;
    _time += min_sleep_time;
  }
}

uint64_t Scheduler::GetTimeNs() const {
  return _time;
}

void Scheduler::WakeUpNeeded() {
  auto iter_to_remove = _sleep_list.end();
  for (auto& elem : _sleep_list) {
    if (elem.first > _time) {
      iter_to_remove = _sleep_list.find(elem.first);
      break;
    }
    _queue.PushAll(elem.second);
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
    current = next;
    TickTime();
    next->Resume();
    if (next->GetState() == detail::fiber::Completed && !next->IsThreadlikeInstanceAlive()) {
      delete next;
    }
  }
  current = nullptr;
}

void Scheduler::RescheduleCurrent() {
  if (current == nullptr) {
    return;
  }
  auto* fiber = current;
  GetScheduler()->_queue.PushBack(static_cast<detail::fiber::BiNodeScheduler*>(fiber));
  fiber->Yield();
}

Scheduler::Scheduler() : _running(false), _time(0) {
}

void Scheduler::SetTickLength(uint32_t tick) {
  _tick_length = tick;
}

void Scheduler::SetRandomListPick(uint32_t k) {
  _random_list_pick = k;
}

void Scheduler::Sleep(uint64_t ns) {
  if (ns <= GetTimeNs()) {
    return;
  }
  detail::fiber::BiList& sleep_list = _sleep_list[ns];
  auto* fiber = current;
  sleep_list.PushBack(static_cast<detail::fiber::BiNodeScheduler*>(fiber));
  Suspend();
  if (_sleep_list.find(ns) != _sleep_list.end()) {
    _sleep_list[ns].DecSize();
    if (_sleep_list[ns].Empty()) {
      _sleep_list.erase(ns);
    }
  }
}
}  // namespace yaclib::fault

namespace yaclib::detail::fiber {

BiNode* PollRandomElementFromList(BiList& list) {
  auto rand_pos = detail::GetRandNumber(fault::_random_list_pick == 0 ? list.GetSize() : fault::_random_list_pick * 2);
  auto* next = list.GetNth(list.GetSize() - fault::_random_list_pick + rand_pos);
  list.Erase(next);
  return next;
}
}  // namespace yaclib::detail::fiber
