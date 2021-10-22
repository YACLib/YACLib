#include <yaclib/coroutines/coroutine.hpp>

namespace yaclib::coroutines {

void Coroutine::operator()() {
  Resume();
}
void Coroutine::Resume() {
  if (IsCompleted()) {
    return;
  }

  _caller_context.SwitchTo(_context);

  if (_exception != nullptr) {
    rethrow_exception(_exception);
  }
}
void Coroutine::Yield() {
  _context.SwitchTo(_caller_context);
}

bool Coroutine::IsCompleted() const {
  return _completed;
}

void Coroutine::Complete() {
  _completed = true;
  _context.SwitchTo(_caller_context);
}

void Coroutine::Trampoline(void* arg) {
  auto* coroutine = (Coroutine*)arg;

  try {
    coroutine->_routine->Call();
  } catch (...) {
    coroutine->_exception = std::current_exception();
  }

  coroutine->Complete();
}

}  // namespace yaclib::coroutines
