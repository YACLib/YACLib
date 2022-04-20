
#include <yaclib/fault/detail/condition/condition_variable.hpp>

namespace yaclib::detail {

void ConditionVariable::notify_one() noexcept {
  YACLIB_INJECT_FAULT(_impl.notify_one());
}

void ConditionVariable::notify_all() noexcept {
  YACLIB_INJECT_FAULT(_impl.notify_all());
}

void ConditionVariable::wait(std::unique_lock<yaclib::detail::Mutex>& lock) noexcept {
  YACLIB_ERROR(!lock.owns_lock(), "Trying to call wait on not owned lock");
  auto* m = lock.release();
  std::unique_lock guard{m->GetImpl(), std::adopt_lock};
  YACLIB_INJECT_FAULT(_impl.wait(guard));
  guard.release();
  lock = std::unique_lock{*m, std::adopt_lock};
}

ConditionVariable::native_handle_type ConditionVariable::native_handle() {
  return _impl.native_handle();
}

}  // namespace yaclib::detail
