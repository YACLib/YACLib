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

void BaseCore::SetInlineCallback(util::Ptr<ITask> callback) {
  _callback = std::move(callback);
  const auto state = _state.exchange(State::HasInlineCallback, std::memory_order_acq_rel);
  if (state == State::HasResult) {
    InlineExecute();
  }
}

void BaseCore::Stop() noexcept {
  _state.store(State::Stopped, std::memory_order_release);
}

bool BaseCore::SetWaitCallback(util::IRef& callback) noexcept {
  if (Ready()) {
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

void BaseCore::InlineExecute() {
  auto& callback = static_cast<BaseCore&>(*_callback);
  callback.InlineCall(this);
  Clean();
}

void BaseCore::Execute() {
  auto& callback = static_cast<BaseCore&>(*_callback);
  callback._caller = this;
  _executor->Execute(callback);
  Clean();
}

void BaseCore::InlineCall(void* /*context*/) {
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
