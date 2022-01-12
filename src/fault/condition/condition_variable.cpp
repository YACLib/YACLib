#include <yaclib/fault/detail/condition/condition_variable.hpp>

namespace yaclib::detail {

void ConditionVariable::notify_one() noexcept {
  YACLIB_INJECT_FAULT(_impl.notify_one());
}

void ConditionVariable::notify_all() noexcept {
  YACLIB_INJECT_FAULT(_impl.notify_all());
}

void ConditionVariable::wait(std::unique_lock<yaclib_std::mutex>& lock) noexcept {
  auto m = lock.release();
  auto guard = std::unique_lock<std::mutex>(m->GetImpl(), std::adopt_lock);
  m->UpdateOwner(yaclib::detail::kInvalidThreadId);
  YACLIB_INJECT_FAULT(_impl.wait(guard));
  guard.release();
  m->UpdateOwner(yaclib_std::this_thread::get_id());
  auto new_lock = std::unique_lock(*m, std::adopt_lock);
  lock.swap(new_lock);
}

ConditionVariable::native_handle_type ConditionVariable::native_handle() {
  return _impl.native_handle();
}

}  // namespace yaclib::detail
