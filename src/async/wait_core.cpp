#include <yaclib/async/detail/wait_core.hpp>

namespace yaclib::detail {

void WaitCore::Wait(std::unique_lock<std::mutex>& guard) {
  while (!is_ready) {
    cv.wait(guard);
  }
}

}  // namespace yaclib::detail
