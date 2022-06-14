#include <yaclib/fault/detail/fiber/fiber_base.hpp>

#include <utility>

namespace yaclib::detail::fiber {

static FiberBase::Id sNextId{0L};

DefaultAllocator FiberBase::sAllocator{};

FiberBase::FiberBase() : _stack{sAllocator}, _id{++sNextId} {
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

void FiberBase::Suspend() {
  _state = Suspended;
  _context.SwitchTo(_caller_context);
}

void FiberBase::Start() {
  _context.Start();
}

void FiberBase::Exit() {
  _state = Completed;
  if (_joining_fiber != nullptr && _threadlike_instance_alive) {
    ScheduleFiber(_joining_fiber);
  }
  _context.Exit(_caller_context);
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

void* FiberBase::GetTls(uint64_t id, std::unordered_map<uint64_t, void*>& defaults) {
  auto it = _tls.find(id);
  if (it == _tls.end()) {
    return defaults[id];
  } else {
    return it->second;
  }
}

void FiberBase::SetTls(uint64_t id, void* value) {
  _tls[id] = value;
}

IStackAllocator& FiberBase::GetAllocator() noexcept {
  return sAllocator;
}

void FiberBase::SetState(FiberState state) noexcept {
  _state = state;
}

}  // namespace yaclib::detail::fiber
