#include <yaclib/fault/detail/thread/fiber_threadlike.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

using namespace std::chrono_literals;

FiberThreadlike::FiberThreadlike() noexcept = default;

void FiberThreadlike::swap(FiberThreadlike& t) noexcept {
  auto* other_impl = t._impl;
  auto other_queue = std::move(t._join_queue);
  t._join_queue = std::move(_join_queue);
  _join_queue = std::move(other_queue);
  t._impl = _impl;
  _impl = other_impl;
}

bool FiberThreadlike::joinable() const noexcept {
  return Scheduler::Current() == nullptr || Scheduler::Current()->GetId() != this->_impl->GetId();
}

void FiberThreadlike::join() {
  while (_impl->GetState() != Completed) {
    _join_queue.Wait();
  }
}

void FiberThreadlike::detach() {
  throw std::bad_function_call();
}

Fiber::Id FiberThreadlike::get_id() const noexcept {
  return _impl->GetId();
}

FiberThreadlike::native_handle_type FiberThreadlike::native_handle() noexcept {
  YACLIB_ERROR(true, "native_hande is not supported for fibers");
}

unsigned int FiberThreadlike::hardware_concurrency() noexcept {
  return std::thread::hardware_concurrency();
}

FiberThreadlike::~FiberThreadlike() {
  if (_impl == nullptr) {
    return;
  }
  _impl->SetThreadlikeInstanceDead();
  if (_impl->GetState() == Completed) {
    delete _impl;
  }
}

FiberThreadlike& FiberThreadlike::operator=(FiberThreadlike&& t) noexcept {
  _impl = t._impl;
  _join_queue = std::move(t._join_queue);
  _impl->SetCompleteCallback(yaclib::MakeFunc([&]() mutable {
    _join_queue.NotifyAll();
  }));
  t._impl = nullptr;
  return *this;
}

FiberThreadlike::FiberThreadlike(FiberThreadlike&& t) noexcept : _impl(t._impl), _join_queue(std::move(t._join_queue)) {
  _impl->SetCompleteCallback(yaclib::MakeFunc([&]() mutable {
    _join_queue.NotifyAll();
  }));
  t._impl = nullptr;
}

}  // namespace yaclib::detail
