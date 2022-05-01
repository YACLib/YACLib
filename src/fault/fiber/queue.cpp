#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail {

void FiberQueue::Wait() {
  auto fiber = Scheduler::Current();
  _queue.push_back(fiber);
  Scheduler::Suspend();
}

void FiberQueue::NotifyAll() {
  std::vector<IntrusivePtr<Fiber>> all;
  all = _queue;
  _queue.clear();
  for (const auto& elem : all) {
    GetScheduler()->Run(elem);
  }
}

void FiberQueue::NotifyOne() {
  if (_queue.empty()) {
    return;
  }
  auto fiber = PollRandomElementFromList(_queue);
  GetScheduler()->Run(fiber);
}

bool FiberQueue::IsEmpty() {
  return _queue.empty();
}
}  // namespace yaclib::detail
