#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/util/cast.hpp>

namespace yaclib::detail {

template <typename V, typename E>
class UniqueCore : public ResultCore<V, E> {
  using ResultCore<V, E>::ResultCore;

  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if constexpr (std::is_move_constructible_v<Result<V, E>>) {
      if constexpr (std::is_copy_constructible_v<Result<V, E>>) {
        if (caller.GetRef() != 1) {
          ResultCore<V, E>::Store(DownCast<ResultCore<V, E>>(caller).Get());
          return SetResult<SymmetricTransfer>();
        }
      }
      YACLIB_ASSERT(caller.GetRef() == 1);
      ResultCore<V, E>::Store(std::move(DownCast<ResultCore<V, E>>(caller).Get()));
      caller.DecRef();
      return SetResult<SymmetricTransfer>();
    } else {
      YACLIB_PURE_VIRTUAL();
      return Noop<SymmetricTransfer>();
    }
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

  void StoreCallback(InlineCore& callback) noexcept {
    return BaseCore::StoreCallbackImpl(callback);
  }

  [[nodiscard]] bool SetCallback(InlineCore& callback) noexcept {
    return BaseCore::SetCallbackImpl<false>(callback);
  }

  // Sometimes we know it will be last callback in cycle, so we want call it right now, instead of SetInline
  void CallInline(InlineCore& callback) noexcept {
    if (!SetCallback(callback)) {
      auto* next = callback.Here(*this);
      YACLIB_ASSERT(next == nullptr);
    }
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetInline(InlineCore& callback) noexcept {
    if (!SetCallback(callback)) {
      return Step<SymmetricTransfer>(*this, callback);
    }
    return Noop<SymmetricTransfer>();
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept {
    return BaseCore::SetResultImpl<SymmetricTransfer, false>();
  }
};

extern template class UniqueCore<void, StopError>;

template <typename V, typename E>
using UniqueCorePtr = IntrusivePtr<UniqueCore<V, E>>;

}  // namespace yaclib::detail
