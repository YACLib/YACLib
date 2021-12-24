#include <yaclib/fault/detail/condition/condition_variable.hpp>

namespace yaclib::detail {

void ConditionVariable::notify_one() noexcept {
  YACLIB_INJECT_FAULT(_impl.notify_one();)
}

void ConditionVariable::notify_all() noexcept {
  YACLIB_INJECT_FAULT(_impl.notify_all();)
}

void ConditionVariable::wait(std::unique_lock<yaclib_std::mutex>& lock) noexcept {
  YACLIB_INJECT_FAULT(_impl.wait(lock);)
}

}  // namespace yaclib::detail
