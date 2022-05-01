#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail {

void FiberQueue::Wait() {
  auto* fiber = Scheduler::Current();
  _queue.PushBack(static_cast<BiNodeWaitQueue*>(fiber));
  Scheduler::Suspend();
}

void FiberQueue::NotifyAll() {
  std::vector<Fiber*> all;
  while (!_queue.Empty()) {
    all.push_back(static_cast<Fiber*>(static_cast<BiNodeWaitQueue*>(_queue.PopBack())));
  }
  for (const auto& elem : all) {
    GetScheduler()->Run(elem);
  }
}

void FiberQueue::NotifyOne() {
  if (_queue.Empty()) {
    return;
  }
  auto* fiber = PollRandomElementFromList(_queue);
  GetScheduler()->Run(static_cast<Fiber*>(static_cast<BiNodeWaitQueue*>(fiber)));
}
}  // namespace yaclib::detail
