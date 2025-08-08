#pragma once

#include <yaclib/algo/detail/result_core.hpp>

namespace yaclib::detail {

template <typename V, typename E>
class SharedCore : public ResultCore<V, E> {
  using ResultCore<V, E>::ResultCore;

 public:
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return ResultCore<V, E>::template Impl<false, true>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return ResultCore<V, E>::template Impl<true, true>(caller);
  }
#endif

  [[nodiscard]] bool SetCallback(InlineCore& callback) noexcept {
    return BaseCore::SetCallbackImpl<true>(callback);
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept {
    return BaseCore::SetResultImpl<SymmetricTransfer, true>();
  }
};

extern template class SharedCore<void, StopError>;

template <typename V, typename E>
using SharedCorePtr = IntrusivePtr<SharedCore<V, E>>;

}  // namespace yaclib::detail
