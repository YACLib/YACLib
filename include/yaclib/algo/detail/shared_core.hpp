#pragma once

#include <yaclib/algo/detail/result_core.hpp>

namespace yaclib::detail {

// 3 refs for the promise (1 for the promise itself and 2 for the last callback)
// 1 ref for the future
inline constexpr size_t kSharedRefWithFuture = 4;
inline constexpr size_t kSharedRefNoFuture = 3;

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

  // Users should be cautious calling SetInline on a SharedCore
  // because the core's lifetime is managed by the SharedPromise and
  // SharedFutures and they might all be gone by the time
  // the callback is called
  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetInline(InlineCore& callback) noexcept {
    return BaseCore::SetInlineImpl<SymmetricTransfer, true>(callback);
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
