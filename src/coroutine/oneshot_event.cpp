#include <yaclib/coroutine/oneshot_event.hpp>

namespace yaclib {

OneShotEvent::OneShotEvent() noexcept : _head{OneShotEvent::kEmpty} {
}

void OneShotEvent::Set() noexcept {
  auto node =
    _head.exchange(OneShotEvent::kAllDone, std::memory_order_seq_cst);  // TSAN warning in case of acq_rel, see later
  while (node != OneShotEvent::kEmpty) {
    Job* job = static_cast<Job*>(reinterpret_cast<detail::Node*>(node));
    node = reinterpret_cast<std::uintptr_t>(job->next);

    job->Call();
  }
}

void OneShotEvent::Reset() noexcept {
  _head.store(OneShotEvent::kEmpty, std::memory_order_release);
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(std::memory_order_acquire) == OneShotEvent::kAllDone;
}

bool OneShotEvent::TryAdd(Job* job) noexcept {
  std::uintptr_t head = _head.load(std::memory_order_acquire);
  std::uintptr_t node = reinterpret_cast<std::uintptr_t>(static_cast<detail::Node*>(job));
  while (head != OneShotEvent::kAllDone) {
    job->next = reinterpret_cast<detail::Node*>(head);
    if (_head.compare_exchange_weak(head, node, std::memory_order_seq_cst,
                                    std::memory_order_seq_cst)) {  // TSAN warning in case of non-seq_cst, see later

      return true;
    }
  }
  return false;
}

OneShotEventAwaiter OneShotEvent::Await(IExecutor& executor) {
  return OneShotEventAwaiter{executor, *this};
}

void OneShotEvent::Wait() {
  detail::NopeCounter<OneShotEventWait> waiter;
  if (TryAdd(static_cast<Job*>(&waiter))) {
    auto token = waiter.Make();
    if (!Ready()) {
      waiter.Wait(token);
    }
  }
}

void Wait(OneShotEvent& event) {
  event.Wait();
}
}  // namespace yaclib
