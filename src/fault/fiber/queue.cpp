#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail::fiber {

WaitStatus FiberQueue::Wait(NoTimeoutTag) {
  auto* fiber = fault::Scheduler::Current();
  _queue.PushBack(static_cast<BiNodeWaitQueue*>(fiber));
  fault::Scheduler::Suspend();
  return WaitStatus::Ready;
}

void FiberQueue::NotifyAll() {
  auto all = std::move(_queue);
  _queue = BiList();
  while (!all.Empty()) {
    auto* fiber = static_cast<FiberBase*>(static_cast<BiNodeWaitQueue*>(all.PopBack()));
    ScheduleAndRemove(fiber);
  }
}

void FiberQueue::NotifyOne() {
  if (_queue.Empty()) {
    return;
  }
  auto* fiber = static_cast<FiberBase*>(static_cast<BiNodeWaitQueue*>(PollRandomElementFromList(_queue)));
  ScheduleAndRemove(fiber);
}

FiberQueue::~FiberQueue() {
  YACLIB_DEBUG(!_queue.Empty(), "queue must be empty on destruction - potentially deadlock");
}

FiberQueue& FiberQueue::operator=(FiberQueue&& other) noexcept {
  _queue = std::move(other._queue);
  other._queue = BiList();
  return *this;
}

bool FiberQueue::Empty() const noexcept {
  return _queue.Empty();
}

void FiberQueue::ScheduleAndRemove(FiberBase* node) {
  if (node->GetState() != Waiting) {
    static_cast<BiNodeScheduler*>(node)->Erase();
    fault::Scheduler::GetScheduler()->Schedule(node);
  }
}

}  // namespace yaclib::detail::fiber
