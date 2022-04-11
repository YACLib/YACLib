
#include <yaclib/fault/detail/mutex/timed_mutex.hpp>

namespace yaclib::detail {

void TimedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());
}

bool TimedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());
  return res;
}

void TimedMutex::unlock() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock());
}

}  // namespace yaclib::detail
