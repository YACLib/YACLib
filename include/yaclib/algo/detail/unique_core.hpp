#pragma once

#include <yaclib/algo/detail/result_core.hpp>

#include <type_traits>

namespace yaclib::detail {

template <typename V, typename E>
class UniqueCore : public ResultCore<V, E> {
  using ResultCore<V, E>::ResultCore;

 public:
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return ResultCore<V, E>::template Impl<false, false>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return ResultCore<V, E>::template Impl<true, false>(caller);
  }
#endif

  Result<V, E> Retire() final {
    if constexpr (std::is_move_constructible_v<Result<V, E>>) {
      auto result = std::move(this->Get());
      this->DecRef();
      return result;
    } else {
      YACLIB_PURE_VIRTUAL();
      return {};
    }
  }

  void StoreCallback(InlineCore& callback) noexcept {
    BaseCore::StoreCallbackImpl(callback);
  }

  [[nodiscard]] bool SetCallback(InlineCore& callback) noexcept {
    return BaseCore::SetCallbackImpl<false>(callback);
  }

  // Sometimes we know it will be last callback in cycle, so we want call it right now, instead of SetInline
  void CallInline(InlineCore& callback) noexcept {
    if (!SetCallback(callback)) {
      [[maybe_unused]] auto* next = callback.Here(*this);
      YACLIB_ASSERT(next == nullptr);
    }
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

extern template class UniqueCore<void, StopError>;

template <typename V, typename E>
using UniqueCorePtr = IntrusivePtr<UniqueCore<V, E>>;

}  // namespace yaclib::detail
