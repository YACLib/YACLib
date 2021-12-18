#include <yaclib/fault/thread.hpp>

namespace yaclib::std::this_thread {

void sleep_for(const ::std::chrono::nanoseconds& time) {
  return ::std::this_thread::sleep_for(time);
}

template <class Duration>
void sleep_until(const ::std::chrono::time_point<yaclib::std::chrono::steady_clock, Duration>& time_point) {
  return ::std::this_thread::sleep_until(time_point);
}

void yield() noexcept {
  return ::std::this_thread::yield();
}

yaclib::std::thread::id get_id() noexcept {
  return ::std::this_thread::get_id();
}

}  // namespace yaclib::std::this_thread
