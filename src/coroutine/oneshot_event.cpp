#include <yaclib/coroutine/oneshot_event.hpp>

namespace yaclib {

OneShotEvent::OneShotEvent() noexcept : _head{OneShotEvent::kEmpty} {
}

void OneShotEvent::Set() noexcept {
  auto node = _head.exchange(OneShotEvent::kAllDone, std::memory_order_acq_rel);
  while (node != OneShotEvent::kEmpty) {
    auto* job = reinterpret_cast<Job*>(node);
    job->Call();
    node = reinterpret_cast<std::uintptr_t>(job->next);
  }
}

void OneShotEvent::Reset() noexcept {
  _head.store(OneShotEvent::kEmpty, std::memory_order_release);
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(std::memory_order_relaxed) == OneShotEvent::kAllDone;
}

bool OneShotEvent::TryAdd(Job* job) noexcept {
  std::uintptr_t head = _head.load(std::memory_order_acquire);
  std::uintptr_t node = reinterpret_cast<std::uintptr_t>(job);
  while (head != OneShotEvent::kAllDone) {
    job->next = reinterpret_cast<Job*>(head);
    if (_head.compare_exchange_weak(head, node, std::memory_order_release, std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

OneShotEventAwaiter OneShotEvent::Await(IExecutor& executor) {
  return OneShotEventAwaiter{executor, *this};
}

void OneShotEvent::Wait() {
  if (Ready()) {
    return;
  }
  detail::NopeCounter<OneShotEventWait> wait;
  if (TryAdd(&wait)) {
    auto token = wait.Make();
    wait.Wait(token);
  }
}

void Wait(OneShotEvent& event) {
  event.Wait();
}
}  // namespace yaclib
