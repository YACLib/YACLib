#include <yaclib/fault/detail/fiber/fiber_base.hpp>

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

#ifdef YACLIB_ASAN
  void* _fake_stack{nullptr};
  const void* _bottom_old{nullptr};
  std::size_t _size_old{0};

  __sanitizer_start_switch_fiber(&_fake_stack, _stack.GetAllocation().start, _stack.GetAllocation().size);
#endif
  _caller_context.SwitchTo(_context);
#ifdef YACLIB_ASAN
  __sanitizer_finish_switch_fiber(_fake_stack, &_bottom_old, &_size_old);
#endif

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
