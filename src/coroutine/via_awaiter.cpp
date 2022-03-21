#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/coroutine/detail/via_awaiter.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>

namespace yaclib::detail {

ViaAwaiter::ViaAwaiter(yaclib::IExecutorPtr e)
    : _executor(std::move(e)), _handle({}), _promise(nullptr), _state(State::Empty) {
}

void ViaAwaiter::IncRef() noexcept {
}
void ViaAwaiter::DecRef() noexcept {
  assert(_state != State::Empty);
  assert(_handle && !_handle.done());
  if (_state == State::HasCalled) {
    _handle.resume();
  } else if (_state == State::HasCancelled) {
    _promise->SetStop();
    _promise->DecRef();
  }
}

void ViaAwaiter::Call() noexcept {
  assert(_state == State::Empty);
  _state = State::HasCalled;
}

void ViaAwaiter::Cancel() noexcept {
  assert(_state == State::Empty);
  _state = State::HasCancelled;
}

bool ViaAwaiter::await_ready() {
  return false;
}

void ViaAwaiter::await_resume() {
}

}  // namespace yaclib::detail
