#include "fault/util.hpp"

#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <iostream>

namespace yaclib::detail {

static thread_local Fiber* current;

Fiber* Scheduler::GetNext() {
  YACLIB_DEBUG(_queue.empty(), "Queue can't be empty");
  auto* next = PollRandomElementFromList(_queue);
  return next;
}

bool Scheduler::IsRunning() const {
  return _running;
}

void Scheduler::Suspend() {
  YACLIB_DEBUG(current == nullptr, "Current can't be null");
  auto* fiber = current;
  fiber->Yield();
}

Scheduler* GetScheduler() {
  thread_local static Scheduler scheduler;
  return &scheduler;
}

Fiber* PollRandomElementFromList(std::vector<Fiber*>& list) {
  auto rand_pos = GetRandNumber(list.size());
  auto* next = list[rand_pos];
  list.erase(list.begin() + rand_pos);
  return next;
}

BiNode* PollRandomElementFromList(BiList& list) {
  auto rand_pos = GetRandNumber(list.GetSize());
  auto* next = list.GetNth(rand_pos);
  list.Erase(next);
  return next;
}

void Scheduler::Run(Fiber* fiber) {
  _queue.push_back(fiber);
  if (!IsRunning()) {
    _running = true;
    RunLoop();
    _running = false;
  }
}

Fiber* Scheduler::Current() {
  return current;
}

Fiber::Id Scheduler::GetId() {
  YACLIB_DEBUG(current == nullptr, "Current can't be null");
  return current->GetId();
}

void Scheduler::TickTime() {
  _time += 10;
}

void Scheduler::AdvanceTime() {
  auto min_sleep_time = _sleep_list.begin()->first - _time;
  if (min_sleep_time >= 0) {
    _time += min_sleep_time;
  }
}

uint64_t Scheduler::GetTimeUs() const {
  return _time;
}

void Scheduler::WakeUpNeeded() {
  auto iter_to_remove = _sleep_list.end();
  for (auto& elem : _sleep_list) {
    if (elem.first > _time) {
      iter_to_remove = _sleep_list.find(elem.first);
      break;
    }
    while (!elem.second.Empty()) {
      _queue.push_back(static_cast<Fiber*>(static_cast<BiNodeSleep*>(elem.second.PopBack())));
    }
  }
  if (iter_to_remove != _sleep_list.begin()) {
    _sleep_list.erase(_sleep_list.begin(), iter_to_remove);
  }
}

void Scheduler::RunLoop() {
  while (!_queue.empty() || !_sleep_list.empty()) {
    if (_queue.empty()) {
      AdvanceTime();
    }
    WakeUpNeeded();
    YACLIB_INFO(_queue.empty(), "Potentially deadlock");
    auto* next = GetNext();
    current = next;
    TickTime();
    next->Resume();
    if (next->GetState() == Completed && !next->IsThreadlikeInstanceAlive()) {
      delete next;
    }
  }
  current = nullptr;
}

void Scheduler::RescheduleCurrent() {
  YACLIB_DEBUG(current == nullptr, "Current can't be null");
  auto* fiber = current;
  GetScheduler()->_queue.push_back(fiber);
  fiber->Yield();
}

Scheduler::Scheduler() : _running(false), _time(0) {
}
}  // namespace yaclib::detail
