#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/fault/injector.hpp>

#include <utility>

namespace yaclib::detail::fiber {

static FiberBase::Id sNextId{1L};

DefaultAllocator FiberBase::sAllocator{};

FiberBase::FiberBase() : _id(sNextId++), _stack(sAllocator) {
}

FiberBase::Id FiberBase::GetId() const noexcept {
  return _id;
}

void FiberBase::Resume() {
  if (_state == Completed) {
    return;
  }

  _state = Running;

  _caller_context.SwitchTo(_context);

  if (_exception != nullptr) {
    rethrow_exception(_exception);
  }
}

void FiberBase::Yield() {
  _state = Suspended;
  _context.SwitchTo(_caller_context);
}

void FiberBase::Complete() {
  _state = Completed;
  if (_joining_fiber != nullptr && _threadlike_instance_alive) {
    ScheduleFiber(_joining_fiber);
  }
  _context.SwitchTo(_caller_context);
}

FiberState FiberBase::GetState() noexcept {
  return _state;
}

void FiberBase::SetJoiningFiber(FiberBase* joining_fiber) noexcept {
  _joining_fiber = joining_fiber;
}

void FiberBase::SetThreadlikeInstanceDead() noexcept {
  _threadlike_instance_alive = false;
}

bool FiberBase::IsThreadlikeInstanceAlive() const noexcept {
  return _threadlike_instance_alive;
}

void FiberBase::GetTls(uint64_t name, void** _default) {
  auto* result = _tls[name];
  if (result != nullptr) {
    *_default = result;
  }
}

void FiberBase::SetTls(uint64_t name, void* value) {
  _tls[name] = value;
}

IStackAllocator& FiberBase::GetAllocator() noexcept {
  return sAllocator;
}

void FiberBase::SetState(FiberState state) noexcept {
  _state = state;
}

}  // namespace yaclib::detail::fiber
