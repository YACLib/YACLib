#include <yaclib/algo/one_shot_event.hpp>

namespace yaclib {

OneShotEvent::OneShotEvent() noexcept : _head{OneShotEvent::kEmpty} {
}

bool OneShotEvent::TryAdd(Job* job) noexcept {
  std::uint64_t head = _head.load(/*std::memory_order_acquire*/);
  std::uint64_t node = reinterpret_cast<std::uint64_t>(job);
  while (head != OneShotEvent::kAllDone) {
    job->next = reinterpret_cast<Job*>(head);
    if (_head.compare_exchange_weak(head, node /*, std::memory_order_release, std::memory_order_acquire*/)) {
      return true;
    }
  }
  return false;
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(/*std::memory_order_acquire*/) == OneShotEvent::kAllDone;
}

// TODO(mkornaukhov03) both Job and DefaultEvent inherited from IRef
class OneShotEventWaiter final : public detail::NopeCounter<Job, detail::DefaultEvent> {
 public:
  void Call() noexcept final {
    Set();
  }

  void Drop() noexcept final {  // LCOV_EXCL_LINE Never called
    Set();                      // LCOV_EXCL_LINE
  }                             // LCOV_EXCL_LINE
};

void OneShotEvent::Wait() noexcept {
  OneShotEventWaiter waiter;
  if (TryAdd(static_cast<Job*>(&waiter))) {
    auto token = waiter.Make();
    waiter.Wait(token);
  }
}

void OneShotEvent::Set() noexcept {
  auto head = _head.exchange(OneShotEvent::kAllDone /*, std::memory_order_acq_rel*/);
  auto job = reinterpret_cast<Job*>(head);
  while (job != nullptr) {
    auto next = static_cast<Job*>(job->next);
    job->Call();
    job = next;
  }
}

void OneShotEvent::Reset() noexcept {
  _head.store(OneShotEvent::kEmpty /*, std::memory_order_release*/);
}

}  // namespace yaclib
