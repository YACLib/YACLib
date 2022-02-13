#include <yaclib/async/detail/wait_core.hpp>
#include <yaclib/fault/condition_variable.hpp>
#include <yaclib/fault/mutex.hpp>

namespace yaclib::detail {

void WaitCore::Wait(std::unique_lock<yaclib_std::mutex>& lock) {
  while (!is_ready) {
    cv.wait(lock);
  }
}

}  // namespace yaclib::detail
