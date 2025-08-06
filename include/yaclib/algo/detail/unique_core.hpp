#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/util/cast.hpp>

namespace yaclib::detail {

template <typename V, typename E>
struct UniqueCore : ResultCore<V, E> {
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

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return Impl<false>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return Impl<true>(caller);
  }
#endif

  void StoreCallback(InlineCore& callback) noexcept {
    return BaseCore::StoreCallbackImpl<false>(callback);
  }

  [[nodiscard]] bool TryAddCallback(InlineCore& callback) noexcept {
    return BaseCore::TryAddCallbackImpl<false>(callback);
  }

  [[nodiscard]] bool Reset() noexcept {
    return BaseCore::ResetImpl<false>();
  }

  void CallInline(InlineCore& callback) noexcept {
    return BaseCore::CallInlineImpl<false>(callback);
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetInline(InlineCore& callback) noexcept {
    return BaseCore::SetInlineImpl<SymmetricTransfer, false>(callback);
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept {
    return BaseCore::SetResultImpl<SymmetricTransfer, false>();
  }
};

template <typename V, typename E>
using UniqueCorePtr = IntrusivePtr<UniqueCore<V, E>>;

}  // namespace yaclib::detail
