#include <yaclib/fault/detail/fiber/thread.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail::fiber {

static unsigned int fiber_hardware_concurrency{std::thread::hardware_concurrency()};

using namespace std::chrono_literals;

Thread::Thread() noexcept = default;

void Thread::swap(Thread& t) noexcept {
  auto* other_impl = t._impl;
  t._impl = _impl;
  _impl = other_impl;
  _joined_or_detached = std::exchange(t._joined_or_detached, _joined_or_detached);
}

bool Thread::joinable() const noexcept {
  return !_joined_or_detached &&
         (fault::Scheduler::Current() == nullptr || fault::Scheduler::Current()->GetId() != this->_impl->GetId());
}

void Thread::join() {
  YACLIB_ERROR(_joined_or_detached, "already joined or detached on join");
  _joined_or_detached = true;
  // TODO(myannyax): allow joining from other threads
  while (_impl->GetState() != Completed) {
    _impl->SetJoiningFiber(fault::Scheduler::GetScheduler()->Current());
    fault::Scheduler::GetScheduler()->Suspend();
  }
  AfterJoinOrDetach();
}

void Thread::detach() {
  _joined_or_detached = true;
  AfterJoinOrDetach();
}

FiberBase::Id Thread::get_id() const noexcept {
  return _impl == nullptr ? FiberBase::Id{0} : _impl->GetId();
}

Thread::native_handle_type Thread::native_handle() noexcept {
  YACLIB_ERROR(true, "native_hande is not supported for fibers");
}

unsigned int Thread::hardware_concurrency() noexcept {
  return fiber_hardware_concurrency;
}

Thread::~Thread() {
  if (_joined_or_detached == false) {
    std::terminate();
  }
}

Thread& Thread::operator=(Thread&& t) noexcept {
  _impl = t._impl;
  _joined_or_detached = t._joined_or_detached;
  t._impl = nullptr;
  t._joined_or_detached = true;
  return *this;
}

Thread::Thread(Thread&& t) noexcept : _impl(t._impl), _joined_or_detached(t._joined_or_detached) {
  t._impl = nullptr;
  t._joined_or_detached = true;
}

void Thread::AfterJoinOrDetach() {
  if (_impl == nullptr) {
    return;
  }
  _impl->SetThreadlikeInstanceDead();
  if (_impl->GetState() == Completed) {
    delete _impl;
  }
  _impl = nullptr;
}

void Thread::SetHardwareConcurrency(unsigned int h_c) noexcept {
  fiber_hardware_concurrency = h_c;
}

}  // namespace yaclib::detail::fiber
