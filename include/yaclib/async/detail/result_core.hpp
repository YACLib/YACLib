#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib::detail {

template <typename Value>
class ResultCore : public BaseCore {
 public:
  void SetResult(util::Result<Value>&& result) {
    _result = std::move(result);
    const auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    if (state == State::HasCallback) {
      BaseCore::Execute();
    } else if (state == State::HasInlineCallback) {
      BaseCore::Execute(*MakeInline());
    } else if (state == State::HasWaitCallback) {
      _callback = nullptr;
    } else if (state == State::Stopped) {
      BaseCore::Cancel();
    }
  }

  util::Result<Value>& Get() noexcept {
    return _result;
  }

 private:
  util::Result<Value> _result;
};

extern template class ResultCore<void>;

}  // namespace yaclib::detail
