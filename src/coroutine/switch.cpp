#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/coroutine/switch.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/thread.hpp>

namespace yaclib::detail {

SwitchAwaiter::SwitchAwaiter(yaclib::IExecutorPtr e)
    : _executor(std::move(e)), _handle({}), _promise(nullptr), _state(State::Empty) {
}

void SwitchAwaiter::IncRef() noexcept {
}
void SwitchAwaiter::DecRef() noexcept {
  assert(_state != State::Empty);
  assert(_handle && !_handle.done());
  if (_state == State::HasCalled) {
    _handle.resume();
  } else if (_state == State::HasCancelled) {
    _promise->SetStop();
    _promise->DecRef();
  }
}

void SwitchAwaiter::Call() noexcept {
  assert(_state == State::Empty);
  _state = State::HasCalled;
}

void SwitchAwaiter::Cancel() noexcept {
  assert(_state == State::Empty);
  _state = State::HasCancelled;
}

bool SwitchAwaiter::await_ready() {
  return false;
}

void SwitchAwaiter::await_resume() {
}

}  // namespace yaclib::detail
