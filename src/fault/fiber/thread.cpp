#include <yaclib/fault/detail/fiber/thread.hpp>
#include <yaclib/log.hpp>

#include <cstdio>
#include <system_error>
#include <utility>

namespace yaclib::detail::fiber {

static unsigned int gHardwareConcurrency{0};

Thread::Thread() noexcept = default;

void Thread::swap(Thread& t) noexcept {
  std::swap(_impl, t._impl);
}

bool Thread::joinable() const noexcept {
  return get_id() != fault::Scheduler::GetId();
}

void Thread::join() {
  if (_impl == nullptr) {
    throw std::system_error{std::make_error_code(std::errc::no_such_process)};
  }
  if (!joinable()) {
    throw std::system_error{std::make_error_code(std::errc::resource_deadlock_would_occur)};
  }

  // TODO(myannyax) allow joining from other threads
  while (_impl->GetState() != Completed) {
    _impl->SetJoiningFiber(fault::Scheduler::Current());
    fault::Scheduler::Suspend();
  }
  AfterJoinOrDetach();
}

void Thread::detach() {
  if (_impl == nullptr) {
    throw std::system_error{std::make_error_code(std::errc::no_such_process)};
  }
  if (!joinable()) {
    throw std::system_error{std::make_error_code(std::errc::resource_deadlock_would_occur)};
  }
  AfterJoinOrDetach();
}

FiberBase::Id Thread::get_id() const noexcept {
  return _impl != nullptr ? _impl->GetId() : FiberBase::Id{0};
}

Thread::native_handle_type Thread::native_handle() noexcept {
  YACLIB_DEBUG(true, "native_hande is not supported for fibers");
  return nullptr;
}

unsigned int Thread::hardware_concurrency() noexcept {
  return gHardwareConcurrency != 0 ? gHardwareConcurrency : std::thread::hardware_concurrency();
}

Thread::~Thread() {
  if (_impl != nullptr) {
    std::terminate();
  }
}

Thread& Thread::operator=(Thread&& t) noexcept {
  swap(t);
  return *this;
}

Thread::Thread(Thread&& t) noexcept : _impl{std::exchange(t._impl, nullptr)} {
}

void Thread::AfterJoinOrDetach() {
  YACLIB_ASSERT(_impl);
  if (_impl->GetState() == Completed) {
    delete _impl;
  } else {
    _impl->SetThreadDead();
  }
  _impl = nullptr;
}

void Thread::SetHardwareConcurrency(unsigned int hardware_concurrency) noexcept {
  gHardwareConcurrency = hardware_concurrency;
}

}  // namespace yaclib::detail::fiber
