#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/util/cast.hpp>

namespace yaclib::detail {

template <typename V, typename E>
class SharedCore : public ResultCore<V, E> {
  using ResultCore<V, E>::ResultCore;

  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if (caller.GetRef() != 1) {
      ResultCore<V, E>::Store(DownCast<ResultCore<V, E>>(caller).Get());
    } else {
      ResultCore<V, E>::Store(std::move(DownCast<ResultCore<V, E>>(caller).Get()));
      caller.DecRef();
    }
    return SetResult<SymmetricTransfer>();
  }

 public:
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return Impl<false>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return Impl<true>(caller);
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
