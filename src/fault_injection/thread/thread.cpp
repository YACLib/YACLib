#include "thread.hpp"

namespace yaclib::std {

Thread::Thread() noexcept : _impl() {
}

Thread::Thread(Thread&& t) noexcept {
  _impl = ::std::thread(t);
}

Thread& Thread::operator=(Thread&& t) noexcept {
  _impl = ::std::thread(t);
  return *this;
}

void Thread::swap(Thread& t) noexcept {
  _impl.swap(t._impl);
}

bool Thread::joinable() const noexcept {
  return _impl.joinable();
}

void Thread::join() {
  _impl.join();
}

void Thread::detach() {
  _impl.detach();
}

Thread::id Thread::get_id() const noexcept {
  return _impl.get_id();
}

std::Thread::native_handle_type Thread::native_handle() noexcept {
  return _impl.native_handle();
}

unsigned Thread::hardware_concurrency() noexcept {
  return ::std::thread::hardware_concurrency();
}

Thread::Thread(::std::function<void()> routine) {
  _impl = ::std::thread(routine);
}

}  // namespace yaclib::std
