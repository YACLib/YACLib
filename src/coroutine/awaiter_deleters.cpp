#include <yaclib/coroutine/detail/await_awaiter.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

void HandleDeleter::Delete(Handle& handle) noexcept {
  YACLIB_DEBUG(!handle.handle, "saved to resume handle is null");
  YACLIB_DEBUG(handle.handle.done(), "handle for resume is done");
  handle.handle.resume();  // TODO(mkornaukhov03) resume on custom IExecutor
}

bool LFStack::AddWaiter(BaseCore* core_ptr) noexcept {
  std::uintptr_t old_head = head.load(std::memory_order_acquire);
  std::uintptr_t core = reinterpret_cast<std::uintptr_t>(core_ptr);
  while (old_head != kAllDone) {
    core_ptr->next = reinterpret_cast<BaseCore*>(old_head);
    if (head.compare_exchange_weak(old_head, core, std::memory_order_release, std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}
void AwaitersResumer::Delete(LFStack& awaiters) noexcept {
  std::uintptr_t old_head = awaiters.head.exchange(LFStack::kAllDone, std::memory_order_acq_rel);  // TODO noexcept?
  int i = 0;
  while (old_head != LFStack::kEmpty) {
    BaseCore* core = reinterpret_cast<BaseCore*>(old_head);
    old_head = reinterpret_cast<std::uintptr_t>(static_cast<BaseCore*>(core->next));
    core->GetExecutor()->Submit(*core);
  }
}

}  // namespace yaclib::detail
