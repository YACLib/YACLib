#include <yaclib/async/detail/base_core.hpp>

#include <cassert>

namespace yaclib::detail {

BaseCore::BaseCore(State state) : _state{state} {
}

IExecutorPtr BaseCore::GetExecutor() const noexcept {
  return _executor;
}

void BaseCore::SetExecutor(IExecutorPtr executor) noexcept {
  _executor = std::move(executor);
}

bool BaseCore::Ready() const noexcept {
  return _state.load(std::memory_order_acquire) == State::HasResult;
}

void BaseCore::SetCallback(util::Ptr<ITask> callback) {
  _callback = std::move(callback);
  const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
  if (state == State::HasResult) {
    Execute();
  }
}

void BaseCore::Stop() noexcept {
  _state.store(State::Stopped, std::memory_order_release);
}

bool BaseCore::SetWaitCallback(util::IRef& callback) noexcept {
  if (_state.load(std::memory_order_acquire) == State::HasResult) {
    return true;
  }
  _callback = &callback;
  auto expected = State::Empty;
  if (!_state.compare_exchange_strong(expected, State::HasWaitCallback, std::memory_order_acq_rel)) {
    _callback = nullptr;  // This is mean we have Result
    return true;
  }
  return false;
}

bool BaseCore::ResetAfterTimeout() noexcept {
  auto expected = State::HasWaitCallback;
  if (_state.compare_exchange_strong(expected, State::Empty, std::memory_order_acq_rel)) {
    _callback = nullptr;  // This is mean we don't have executed callback
    return true;
  }
  return false;
}

void BaseCore::Execute() {
  static_cast<BaseCore&>(*_callback)._caller = this;
  Execute(*_executor);
}

void BaseCore::Execute(IExecutor& e) {
  e.Execute(static_cast<ITask&>(*_callback));
  Clean();
}

void BaseCore::Call() noexcept {
}

void BaseCore::Cancel() noexcept {  // Opposite for Call with SetResult
  _caller = nullptr;
  Clean();
}

void BaseCore::Clean() noexcept {
  // Order is matter TODO(MBkkt) Why?
  _executor = nullptr;
  _callback = nullptr;
}

}  // namespace yaclib::detail
