#include <yaclib/fault/detail/fiber/condition_variable.hpp>

namespace yaclib::detail::fiber {

void ConditionVariable::notify_one() noexcept {
  _queue.NotifyOne();
}

void ConditionVariable::notify_all() noexcept {
  _queue.NotifyAll();
}

void ConditionVariable::wait(std::unique_lock<yaclib::detail::fiber::Mutex>& lock) noexcept {
  WaitImpl(lock, NoTimeoutTag{});
}

ConditionVariable::native_handle_type ConditionVariable::native_handle() {
  return nullptr;
}

}  // namespace yaclib::detail::fiber
