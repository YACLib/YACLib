#include <yaclib/coroutine/oneshot_event.hpp>

namespace yaclib {

OneShotEvent::OneShotEvent() noexcept : _head{OneShotEvent::kEmpty} {
}

void OneShotEvent::Set() noexcept {
  // TSAN warning
  auto head = _head.exchange(OneShotEvent::kAllDone
                             /*, std::memory_order_acq_rel*/);
  auto job = static_cast<Job*>(reinterpret_cast<detail::Node*>(head));
  while (job != nullptr) {
    auto next = static_cast<Job*>(job->next);
    job->Call();
    job = next;
  }
}

void OneShotEvent::Reset() noexcept {
  _head.store(OneShotEvent::kEmpty /*, std::memory_order_release*/);
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(/*std::memory_order_acquire*/) == OneShotEvent::kAllDone;
}

bool OneShotEvent::TryAdd(Job* job) noexcept {
  std::uintptr_t head = _head.load(/*std::memory_order_acquire */);
  std::uintptr_t node = reinterpret_cast<std::uintptr_t>(static_cast<detail::Node*>(job));
  while (head != OneShotEvent::kAllDone) {
    job->next = reinterpret_cast<detail::Node*>(head);
    // TSAN warning
    if (_head.compare_exchange_weak(head, node /*, std::memory_order_release, std::memory_order_acquire*/)) {
      return true;
    }
  }
  return false;
}

detail::NopeCounter<OneShotEventAwaiter> OneShotEvent::Await(IExecutor& executor) noexcept {
  return detail::NopeCounter<OneShotEventAwaiter>{executor, *this};
}

void OneShotEvent::Wait() {
  detail::NopeCounter<OneShotEventWait> waiter;
  if (TryAdd(static_cast<Job*>(&waiter))) {
    auto token = waiter.Make();
    waiter.Wait(token);
  }
}

void Wait(OneShotEvent& event) {
  event.Wait();
}

}  // namespace yaclib
