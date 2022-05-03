#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/inject.hpp>
#include <yaclib/fault/injector.hpp>

#include <utility>

namespace yaclib::detail::fiber {

static FiberBase::Id next_id{1L};

DefaultAllocator FiberBase::allocator{};

FiberBase::FiberBase() : _id(next_id++), _stack(allocator) {
}

FiberBase::Id FiberBase::GetId() const {
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
    GetInjector()->SetPauseInject(true);
    ScheduleFiber(_joining_fiber);
    GetInjector()->SetPauseInject(false);
  }
  _context.SwitchTo(_caller_context);
}

FiberState FiberBase::GetState() {
  return _state;
}

void FiberBase::SetJoiningFiber(FiberBase* joining_fiber) {
  _joining_fiber = joining_fiber;
}

void FiberBase::SetThreadlikeInstanceDead() {
  _threadlike_instance_alive = false;
}

bool FiberBase::IsThreadlikeInstanceAlive() const {
  return _threadlike_instance_alive;
}

void* FiberBase::GetTls(uint64_t name, void* _default) {
  return _tls[name] == nullptr ? _default : _tls[name];
}

void FiberBase::SetTls(uint64_t name, void* value) {
  _tls[name] = value;
}

IStackAllocator& FiberBase::GetAllocator() {
  return allocator;
}

}  // namespace yaclib::detail::fiber
