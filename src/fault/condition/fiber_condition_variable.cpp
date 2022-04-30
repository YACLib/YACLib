#include <yaclib/fault/detail/condition/fiber_condition_variable.hpp>

namespace yaclib {

void detail::FiberConditionVariable::notify_one() noexcept {
  _queue.NotifyOne();
}

void detail::FiberConditionVariable::notify_all() noexcept {
  _queue.NotifyAll();
}

void detail::FiberConditionVariable::wait(std::unique_lock<yaclib::detail::FiberMutex>& lock) noexcept {
  lock.unlock();
  _queue.Wait();
  lock.lock();
}

detail::FiberConditionVariable::native_handle_type detail::FiberConditionVariable::native_handle() {
  return nullptr;
}
}  // namespace yaclib
