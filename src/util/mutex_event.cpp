
#include <yaclib/util/detail/mutex_event.hpp>

namespace yaclib::detail {

MutexEvent::Token MutexEvent::Make() noexcept {
  return Token{_m};
}

void MutexEvent::Wait(Token& token) noexcept {
  while (!_is_ready) {
    _cv.wait(token);
  }
}

void MutexEvent::Reset() noexcept {
  _is_ready = false;
}

void MutexEvent::SetOne() noexcept {
  std::lock_guard lock{_m};
  _is_ready = true;
  _cv.notify_one();  // Notify under mutex, because cv located on stack memory of other thread
}

void MutexEvent::SetAll() noexcept {
  std::lock_guard lock{_m};
  _is_ready = true;
  _cv.notify_all();  // Notify under mutex, because cv located on stack memory of other thread
}

}  // namespace yaclib::detail
