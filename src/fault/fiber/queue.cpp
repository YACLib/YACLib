#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail::fiber {

bool FiberQueue::Wait(NoTimeoutTag) {
  auto* fiber = fault::Scheduler::Current();
  _queue.PushBack(static_cast<BiNodeWaitQueue*>(fiber));
  fault::Scheduler::Suspend();
  return true;
}

void FiberQueue::NotifyAll() {
  auto all = std::move(_queue);
  _queue = BiList();
  while (!all.Empty()) {
    fault::Scheduler::GetScheduler()->Schedule(static_cast<FiberBase*>(static_cast<BiNodeWaitQueue*>(all.PopBack())));
  }
}

void FiberQueue::NotifyOne() {
  if (_queue.Empty()) {
    return;
  }
  auto* fiber = PollRandomElementFromList(_queue);
  fault::Scheduler::GetScheduler()->Schedule(static_cast<FiberBase*>(static_cast<BiNodeWaitQueue*>(fiber)));
}

FiberQueue::~FiberQueue() {
  YACLIB_ERROR(!_queue.Empty(), "queue must be empty on destruction - potentially deadlock");
}

FiberQueue& FiberQueue::operator=(FiberQueue&& other) noexcept {
  _queue = std::move(other._queue);
  other._queue = BiList();
  return *this;
}

bool FiberQueue::Empty() const {
  return _queue.Empty();
}
}  // namespace yaclib::detail::fiber
