#include <yaclib/fault/detail/thread/fiber_threadlike.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

using namespace std::chrono_literals;

FiberThreadlike::FiberThreadlike() noexcept = default;

void FiberThreadlike::swap(FiberThreadlike& t) noexcept {
  auto other_impl = t._impl;
  t._impl = _impl;
  _impl = other_impl;
}

bool FiberThreadlike::joinable() const noexcept {
  return Scheduler::Current() == nullptr || Scheduler::Current()->GetId() != this->_impl->GetId();
}

void FiberThreadlike::join() {
  while (_impl->GetState() != Completed) {
    _join_queue->Wait();
    if (_join_queue->IsEmpty()) {
      delete _join_queue;
    }
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

const Fiber::Id kInvalidThreadId = Fiber::Id{0};

}  // namespace yaclib::detail
