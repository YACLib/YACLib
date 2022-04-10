#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/coroutine/detail/via_awaiter.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>

namespace yaclib::detail {

ViaAwaiter::ViaAwaiter(IExecutorPtr e) : _executor(std::move(e)), _handle({}), _promise(nullptr), _state(State::Empty) {
}

void ViaAwaiter::IncRef() noexcept {
}
void ViaAwaiter::DecRef() noexcept {
  assert(_state != State::Empty);
  if (_state == State::HasCalled) {
    _handle.resume();
  }
}

void ViaAwaiter::Call() noexcept {
  assert(_state == State::Empty);
  _state = State::HasCalled;
}

void ViaAwaiter::Cancel() noexcept {
  assert(_state == State::Empty);
  _state = State::HasCancelled;
  _promise->SetStop();
  _promise->DecRef();
}

bool ViaAwaiter::await_ready() {
  return false;
}

void ViaAwaiter::await_resume() {
}

}  // namespace yaclib::detail
