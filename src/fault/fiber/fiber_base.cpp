#include <yaclib/fault/detail/fiber/fiber_base.hpp>

#include <cstdio>
#include <utility>

namespace yaclib::detail::fiber {

static FiberBase::Id sNextId = 1;

static DefaultAllocator sAllocator;

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
  if (_joining_fiber != nullptr && _thread_alive) {
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

void FiberBase::SetThreadDead() noexcept {
  _thread_alive = false;
}

bool FiberBase::IsThreadAlive() const noexcept {
  return _thread_alive;
}

void* FiberBase::GetTLS(std::uint64_t id, std::unordered_map<std::uint64_t, void*>& defaults) {
  auto it = _tls.find(id);
  if (it == _tls.end()) {
    return defaults[id];
  }
  return it->second;
}

void FiberBase::SetTLS(std::uint64_t id, void* value) {
  _tls[id] = value;
}

IStackAllocator& FiberBase::GetAllocator() noexcept {
  return sAllocator;
}

void FiberBase::SetState(FiberState state) noexcept {
  _state = state;
}

}  // namespace yaclib::detail::fiber
