#pragma once

#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <utility>

namespace yaclib::detail {

template <typename V, typename E>
class ResultCore : public BaseCore {
 public:
  ResultCore() noexcept : BaseCore{State::Empty} {
  }

  template <typename T>
  explicit ResultCore(T&& value) : BaseCore{State::HasResult}, _result{std::forward<T>(value)} {
  }

  template <typename T>
  void Set(T&& value) noexcept {
    _result = std::forward<T>(value);
    _caller = nullptr;  // Order is matter, because we want move value before destroy object that owned value
    const auto state = _state.exchange(State::HasResult, std::memory_order_acq_rel);
    switch (state) {  //  TODO(MBkkt) rel + fence(acquire) instead of acq_rel
      case State::HasCallback:
        return Submit();
      case State::HasCallbackInline:
        [[fallthrough]];
      case State::HasAsyncCallback:
        static_cast<InlineCore&>(*_callback).CallInline(this, state);
        [[fallthrough]];
      case State::HasStop:
        _executor = nullptr;
        [[fallthrough]];
      case State::HasWait:
        _callback = nullptr;
        [[fallthrough]];
      default:
        return;
    }
  }

  [[nodiscard]] Result<V, E>& Get() noexcept {
    return _result;
  }

 private:
  Result<V, E> _result;
};

struct SubscribeTag {};

template <typename E>
class ResultCore<SubscribeTag, E> : public BaseCore {
 public:
  ResultCore() noexcept : BaseCore{State::Empty} {
  }

  template <typename T>
  void Set(T&&) noexcept {
    BaseCore::Cancel();
  }
};

// extern template class ResultCore<void, void>;
extern template class ResultCore<void, StopError>;

template <typename V, typename E>
using ResultCorePtr = IntrusivePtr<ResultCore<V, E>>;

}  // namespace yaclib::detail
