#include <yaclib/fault/detail/fiber/fiber.hpp>

namespace yaclib::detail {

Fiber::Fiber(IStackAllocator& allocator, Routine routine)
    : _stack(allocator), _impl(_stack.GetAllocation(), std::move(routine)), _id(_next_id++) {
}

Fiber::Fiber(Routine routine) : Fiber(gDefaultAllocator, std::move(routine)) {
}

void Fiber::Resume() {
  _state = Running;
  _impl.Resume();
}

void Fiber::Yield() {
  _impl.Yield();
}

Fiber::Id Fiber::GetId() {
  return _id;
}

Fiber::Id Fiber::_next_id = 1L;

void Fiber::IncRef() noexcept {
}
void Fiber::DecRef() noexcept {
}

}  // namespace yaclib::detail
