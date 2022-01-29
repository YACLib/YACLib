#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib::detail {

template <typename Value>
class ResultCore : public BaseCore {
 public:
  ResultCore() noexcept : BaseCore{State::Empty} {
  }

  template <typename T>
  explicit ResultCore(T&& value) : BaseCore{State::HasResult}, _result{std::forward<T>(value)} {
  }

  template <typename T>
  void Set(T&& value) noexcept {
    _caller = nullptr;
    _result = std::forward<T>(value);
    const auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    switch (state) {
      case State::HasCallback:
        BaseCore::Execute();
        break;
      case State::HasCallbackInline:
        BaseCore::ExecuteInline();
        break;
      case State::HasStop:
        _executor = nullptr;
        [[fallthrough]];
      case State::HasWait:
        _callback = nullptr;
        break;
      default:
        break;
    }
  }

  [[nodiscard]] util::Result<Value>& Get() noexcept {
    return _result;
  }

 private:
  void CallInline(InlineCore* caller) noexcept override {
    if (BaseCore::GetState() == BaseCore::State::HasStop) {
      // Don't need to call Cancel, because we call Clean after CallInline and our _caller is nullptr
      return;
    }
    Set(std::move(static_cast<ResultCore<Value>*>(caller)->Get()));
  }

  util::Result<Value> _result;
};

extern template class ResultCore<void>;

}  // namespace yaclib::detail
