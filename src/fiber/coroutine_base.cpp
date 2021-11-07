#include <yaclib/fiber/coroutine_base.hpp>

namespace yaclib {

CoroutineBase::CoroutineBase(const StackView& stack_view, Routine routine) : _routine(std::move(routine)) {
  _context.Setup(stack_view, Trampoline, this);
}

void CoroutineBase::Resume() {
  if (IsCompleted()) {
    return;
  }

  _caller_context.SwitchTo(_context);

  if (_exception != nullptr) {
    rethrow_exception(_exception);
  }
}

void CoroutineBase::Yield() {
  _context.SwitchTo(_caller_context);
}

bool CoroutineBase::IsCompleted() const {
  return _completed;
}

void CoroutineBase::Complete() {
  _completed = true;
  _context.SwitchTo(_caller_context);
}

void CoroutineBase::Trampoline(void* arg) {
  auto* coroutine = reinterpret_cast<CoroutineBase*>(arg);

  try {
    coroutine->_routine->Call();
  } catch (...) {
    coroutine->_exception = std::current_exception();
  }

  coroutine->Complete();
}

}  // namespace yaclib
