#include <yaclib/fault/detail/fiber/scheduler.hpp>

#include <utility>

namespace yaclib::detail {

IntrusivePtr<Fiber> Scheduler::GetNext() {
  YACLIB_ERROR(_queue.empty(), "queue can't be empty");
  auto next = PollRandomElementFromList(_queue);
  return next;
}

bool Scheduler::IsRunning() const {
  return _running;
}

void Scheduler::Suspend() {
  YACLIB_ERROR(current == nullptr, "current can't be null");
  auto fiber = current;
  fiber->_state = Suspended;
  fiber->Yield();
}

Scheduler* GetScheduler() {
  thread_local static Scheduler scheduler;
  return &scheduler;
}

IntrusivePtr<Fiber> PollRandomElementFromList(std::vector<IntrusivePtr<Fiber>>& list) {
  auto rand_pos = GetScheduler()->GetRandNumber() % list.size();
  auto next = list[rand_pos];
  list.erase(list.begin() + rand_pos);
  return next;
}

void Scheduler::Run(IntrusivePtr<Fiber> fiber) {
  _queue.push_back(fiber);
  if (!IsRunning()) {
    _running = true;
    RunLoop();
    _running = false;
  }
}

IntrusivePtr<Fiber> Scheduler::Current() {
  return current;
}

Fiber::Id Scheduler::GetId() {
  YACLIB_ERROR(current == nullptr, "current can't be null");
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

unsigned long Scheduler::GetTimeUs() const {
  return _time;
}

void Scheduler::WakeUpNeeded() {
  auto iter_to_remove = _sleep_list.end();
  for (auto& elem : _sleep_list) {
    if (elem.first > _time) {
      iter_to_remove = _sleep_list.find(elem.first);
      break;
    }
    for (auto& fiber : elem.second) {
      _queue.push_back(fiber);
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
    YACLIB_INFO(_queue.empty(), "potentially deadlock");
    auto next = GetNext();
    current = next;
    TickTime();
    next->Resume();
  }
  current = nullptr;
}

unsigned long Scheduler::GetRandNumber() {
  return _rand();
}

void Scheduler::Run(Routine routine) {
  auto fiber = IntrusivePtr<Fiber>(new Fiber(std::move(routine)));
  _queue.push_back(fiber);
  if (!IsRunning()) {
    _running = true;
    RunLoop();
    _running = false;
  }
}

void Scheduler::RescheduleCurrent() {
  YACLIB_ERROR(current == nullptr, "current can't be null");
  auto fiber = current;
  GetScheduler()->_queue.push_back(fiber);
  fiber->Yield();
}

void Scheduler::Unschedule(IntrusivePtr<Fiber> fiber) {
  _queue.erase(std::remove_if(_queue.begin(), _queue.end(),
                              [&](auto& item) {
                                return item == fiber;
                              }),
               _queue.end());
  std::vector<long> to_delete;
  for (auto& elem : _sleep_list) {
    elem.second.erase(std::remove_if(elem.second.begin(), elem.second.end(),
                                     [&](const auto& item) {
                                       return item == fiber;
                                     }),
                      elem.second.end());
    if (elem.second.empty()) {
      to_delete.push_back(elem.first);
    }
  }
  for (auto elem : to_delete) {
    _sleep_list.erase(elem);
  }
}
}  // namespace yaclib::detail
