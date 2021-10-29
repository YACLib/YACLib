#include "coroutine_impl.hpp"

namespace yaclib {

void CoroutineImpl::operator()() {
  Resume();
}
void CoroutineImpl::Resume() {
  if (IsCompleted()) {
    return;
  }

  _caller_context.SwitchTo(_context);

  if (_exception != nullptr) {
    rethrow_exception(_exception);
  }
}
void CoroutineImpl::Yield() {
  _context.SwitchTo(_caller_context);
}

bool CoroutineImpl::IsCompleted() const {
  return _completed;
}

void CoroutineImpl::Complete() {
  _completed = true;
  _context.SwitchTo(_caller_context);
}

void CoroutineImpl::Trampoline(void* arg) {
  auto* coroutine = reinterpret_cast<CoroutineImpl*>(arg);

  try {
    coroutine->_routine->Call();
  } catch (...) {
    coroutine->_exception = std::current_exception();
  }

  coroutine->Complete();
}

}  // namespace yaclib
