#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <utility>

namespace yaclib::detail {

void InlineCore::Call() noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

void InlineCore::Cancel() noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

void InlineCore::CallInline(InlineCore&, InlineCore::State) noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

BaseCore::BaseCore(State state) noexcept : _state{state} {
}

void BaseCore::SetExecutor(IExecutor* executor) noexcept {
  _executor = executor;
}

IExecutor* BaseCore::GetExecutor() const noexcept {
  return _executor.Get();
}

void BaseCore::SetCallback(BaseCore& callback) noexcept {
  YACLIB_DEBUG(_callback != nullptr, "callback not empty, so future already used");
  _callback.Reset(NoRefTag{}, &callback);
  const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
  if (state == State::HasResult) {  //  TODO(MBkkt) rel + fence(acquire) instead of acq_rel
    Submit();
  }
}

void BaseCore::SetCallbackInline(InlineCore& callback, bool async) noexcept {
  YACLIB_DEBUG(_callback != nullptr, "callback not empty, so future already used");
  _callback.Reset(NoRefTag{}, &callback);
  const auto new_state =
    State{static_cast<char>(static_cast<char>(State::HasCallbackInline) + static_cast<char>(async))};
  const auto old_state = _state.exchange(new_state, std::memory_order_acq_rel);
  if (old_state == State::HasResult) {  //  TODO(MBkkt) rel + fence(acquire) instead of acq_rel
    static_cast<InlineCore&>(*_callback).CallInline(*this, new_state);
  }
}

bool BaseCore::SetWait(IRef& callback) noexcept {
  YACLIB_DEBUG(_callback != nullptr, "callback not empty, so future already used");
  _callback.Reset(NoRefTag{}, &callback);
  auto expected = State::Empty;
  if (!_state.compare_exchange_strong(expected, State::HasWait, std::memory_order_acq_rel)) {
    _callback.Reset(NoRefTag{}, nullptr);  // This is mean we have Result
    return false;
  }
  return true;
}

bool BaseCore::ResetWait() noexcept {
  auto expected = State::HasWait;
  if (_state.load(std::memory_order_acquire) == expected &&
      _state.compare_exchange_strong(expected, State::Empty, std::memory_order_acq_rel)) {
    YACLIB_DEBUG(_callback == nullptr, "callback empty, but should be our wait function");
    _callback.Reset(NoRefTag{}, nullptr);  // This is mean we don't have executed callback
    return true;
  }
  return false;
}

void BaseCore::Stop() noexcept {
  YACLIB_DEBUG(_callback != nullptr, "callback not empty, so future already used");
  _state.store(State::HasStop, std::memory_order_release);
}

bool BaseCore::Empty() const noexcept {
  YACLIB_DEBUG(_callback != nullptr, "callback not empty, so future already used");
  return _state.load(std::memory_order_acquire) == State::Empty;
}

bool BaseCore::Alive() const noexcept {
  return _state.load(std::memory_order_acquire) != State::HasStop;
}

void BaseCore::Submit() noexcept {
  YACLIB_DEBUG(_executor == nullptr, "we try to submit callback to executor, but don't see valid executor");
  YACLIB_DEBUG(_callback == nullptr, "we try to submit callback to executor, but don't see valid callback");
  auto& callback = static_cast<BaseCore&>(*_callback.Release());
  // We create strong cyclic reference, but we know we remove reference from callback to caller in Call or Cancel
  callback._caller = this;  // TODO(MBkkt) We want store without IncRef, destructive move
  _executor->Submit(callback);
}

}  // namespace yaclib::detail
