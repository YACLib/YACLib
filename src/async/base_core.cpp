#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cassert>
#include <utility>

namespace yaclib::detail {

void InlineCore::Call() noexcept {
  YACLIB_ERROR(true, "Pure virtual call");
}
void InlineCore::Cancel() noexcept {
  YACLIB_ERROR(true, "Pure virtual call");
}
void InlineCore::CallInline(InlineCore&, InlineCore::State) noexcept {
  YACLIB_ERROR(true, "Pure virtual call");
}

BaseCore::BaseCore(State state) noexcept : _state{state} {
}

void BaseCore::SetExecutor(IExecutorPtr executor) noexcept {
  _executor = std::move(executor);
}
const IExecutorPtr& BaseCore::GetExecutor() const noexcept {
  return _executor;
}

void BaseCore::SetCallback(BaseCore& callback) noexcept {
  assert(_callback == nullptr);
  _callback.Reset(NoRefTag{}, &callback);
  const auto state = _state.exchange(State::HasCallback, std::memory_order_acq_rel);
  if (state == State::HasResult) {  //  TODO(MBkkt) rel + fence(acquire) instead of acq_rel
    Submit();
  }
}
void BaseCore::SetCallbackInline(InlineCore& callback, bool async) noexcept {
  assert(_callback == nullptr);
  _callback.Reset(NoRefTag{}, &callback);
  const auto new_state = State{static_cast<int>(State::HasCallbackInline) + static_cast<int>(async)};
  const auto old_state = _state.exchange(new_state, std::memory_order_acq_rel);
  if (old_state == State::HasResult) {  //  TODO(MBkkt) rel + fence(acquire) instead of acq_rel
    static_cast<InlineCore&>(*_callback).CallInline(*this, new_state);
    Clean();
  }
}
bool BaseCore::SetWait(IRef& callback) noexcept {
  assert(_callback == nullptr);
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
    assert(_callback != nullptr);
    _callback.Reset(NoRefTag{}, nullptr);  // This is mean we don't have executed callback
    return true;
  }
  return false;
}

void BaseCore::Stop() noexcept {
  assert(_callback == nullptr);
  _state.store(State::HasStop, std::memory_order_release);
  // _executor = nullptr; Don't really need, because this object will be destroyed
}
bool BaseCore::Empty() const noexcept {
  assert(_callback == nullptr);
  return _state.load(std::memory_order_acquire) == State::Empty;
}
bool BaseCore::Alive() const noexcept {
  return _state.load(std::memory_order_acquire) != State::HasStop;
}

void BaseCore::Submit() noexcept {
  assert(_callback != nullptr);
  assert(_executor != nullptr);
  auto& callback = static_cast<BaseCore&>(*_callback.Release());
  callback._caller = this;  // TODO(MBkkt) We want store without IncRef, destructive move
  _executor->Submit(callback);
  Clean();
}

void BaseCore::Cancel() noexcept {  // Opposite for Call with SetResult
  _caller = nullptr;
  Clean();
  this->DecRef();
}

void BaseCore::Clean() noexcept {
  // Order is matter TODO(MBkkt) Why?
  _executor = nullptr;
  _callback = nullptr;
}

}  // namespace yaclib::detail
