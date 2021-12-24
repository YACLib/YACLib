#include <yaclib/fault/detail/thread/thread.hpp>
#include <yaclib/fault/thread.hpp>

namespace yaclib::detail {

Thread::Thread() noexcept : _impl() {
}

Thread::Thread(Thread&& t) noexcept {
  _impl = static_cast<std::thread&&>(t._impl);
}

Thread& Thread::operator=(Thread&& t) noexcept {
  _impl = static_cast<std::thread&&>(t._impl);
  return *this;
}

void Thread::swap(Thread& t) noexcept {
  _impl.swap(t._impl);
}

bool Thread::joinable() const noexcept {
  return _impl.joinable();
}

void Thread::join() {
  assert(get_id() != yaclib_std::this_thread::get_id());
  _impl.join();
}

void Thread::detach() {
  _impl.detach();
}

Thread::id Thread::get_id() const noexcept {
  return _impl.get_id();
}

Thread::native_handle_type Thread::native_handle() noexcept {
  return _impl.native_handle();
}

auto Thread::hardware_concurrency() noexcept {
  return std::thread::hardware_concurrency();
}

const Thread::id kInvalidThreadId = std::thread::id{};

}  // namespace yaclib::detail
