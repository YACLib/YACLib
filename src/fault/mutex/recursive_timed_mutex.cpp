#include <yaclib/config.hpp>
#include <yaclib/fault/detail/mutex/recursive_timed_mutex.hpp>

namespace yaclib::detail {

void RecursiveTimedMutex::lock() {
  YACLIB_INJECT_FAULT(_m.lock());
}

bool RecursiveTimedMutex::try_lock() noexcept {
  YACLIB_INJECT_FAULT(auto res = _m.try_lock());
  return res;
}

void RecursiveTimedMutex::unlock() noexcept {
  YACLIB_INJECT_FAULT(_m.unlock());
}

}  // namespace yaclib::detail
