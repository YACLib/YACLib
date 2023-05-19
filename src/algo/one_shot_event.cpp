#include <yaclib/algo/one_shot_event.hpp>

namespace yaclib {
namespace {

void SetImpl(yaclib_std::atomic_uintptr_t& self, std::uintptr_t value) {
  auto head = self.exchange(value, std::memory_order_acq_rel);
  auto* job = reinterpret_cast<Job*>(head);
  while (job != nullptr) {
    auto* next = static_cast<Job*>(job->next);
    job->Call();
    job = next;
  }
}

}  // namespace

bool OneShotEvent::TryAdd(Job& job) noexcept {
  auto head = _head.load(std::memory_order_acquire);
  auto node = reinterpret_cast<std::uintptr_t>(&job);
  while (head != OneShotEvent::kAllDone) {
    job.next = reinterpret_cast<Job*>(head);
    if (_head.compare_exchange_weak(head, node, std::memory_order_release, std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

bool OneShotEvent::Ready() noexcept {
  return _head.load(std::memory_order_acquire) == OneShotEvent::kAllDone;
}

void OneShotEvent::Wait() noexcept {
  Waiter waiter;
  if (TryAdd(waiter)) {
    auto token = waiter.Make();
    waiter.Wait(token);
  }
}

void OneShotEvent::Call() noexcept {
  SetImpl(_head, kEmpty);
}

void OneShotEvent::Set() noexcept {
  SetImpl(_head, kAllDone);
}

void OneShotEvent::Reset() noexcept {
#ifdef YACLIB_LOG_DEBUG
  auto head = _head.load(std::memory_order_relaxed);
  YACLIB_ASSERT(head == kEmpty || head == kAllDone);
#endif
  _head.store(kEmpty, std::memory_order_relaxed);
}

}  // namespace yaclib
