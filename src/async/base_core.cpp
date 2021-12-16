#include "yaclib/async/detail/inline_core.hpp"

#include <yaclib/async/detail/base_core.hpp>

namespace yaclib::detail {

void InlineCore::Call() noexcept {
}
void InlineCore::Cancel() noexcept {
}

BaseCore::BaseCore(State state) noexcept : _state{state} {
}

IExecutorPtr BaseCore::GetExecutor() const noexcept {
  return _executor;
}
void BaseCore::SetExecutor(IExecutorPtr executor) noexcept {
  _executor = std::move(executor);
}

BaseCore::State BaseCore::GetState() const noexcept {
  return _state.load(std::memory_order_acquire);
}
void BaseCore::SetState(State s) noexcept {
  _state.store(s, std::memory_order_release);
}

void BaseCore::SetCallback(util::Ptr<BaseCore> callback) {
  _callback = std::move(callback);
  const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
  if (state == State::HasResult) {
    Execute();
  }
}
void BaseCore::SetCallbackInline(util::Ptr<InlineCore> callback) {
  _callback = std::move(callback);
  const auto state = _state.exchange(State::HasCallbackInline, std::memory_order_acq_rel);
  if (state == State::HasResult) {
    ExecuteInline();
  }
}
bool BaseCore::SetWait(util::IRef& callback) noexcept {
  if (GetState() == State::HasResult) {
    return true;
  }
  _callback = &callback;
  auto expected = State::Empty;
  if (!_state.compare_exchange_strong(expected, State::HasWait, std::memory_order_acq_rel)) {
    _callback = nullptr;  // This is mean we have Result
    return true;
  }
  return false;
}
bool BaseCore::ResetWait() noexcept {
  auto expected = State::HasWait;
  if (_state.compare_exchange_strong(expected, State::Empty, std::memory_order_acq_rel)) {
    _callback = nullptr;  // This is mean we don't have executed callback
    return true;
  }
  return false;
}

void BaseCore::Execute() noexcept {
  auto& callback = static_cast<BaseCore&>(*_callback);
  callback._caller = this;
  _executor->Execute(callback);
  Clean();
}

void BaseCore::ExecuteInline() noexcept {
  static_cast<InlineCore&>(*_callback).CallInline(this);
  Clean();
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
