#pragma once

#include <yaclib/algo/detail/result_core.hpp>

namespace yaclib::detail {

// 1 ref for the promise, 1 ref for the future.
// Every attached callback additionally owns 1 ref, granted at attach,
// released by the callback when it is done reading the core
inline constexpr std::size_t kSharedRefWithFuture = 2;
inline constexpr std::size_t kSharedRefNoFuture = 1;

template <typename V, typename T>
class SharedCore : public ResultCore<V, T> {
  using ResultCore<V, T>::ResultCore;

 public:
  using Result = typename ResultCore<V, T>::Result;

  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return ResultCore<V, T>::template Impl<false, true>(caller);
  }

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return ResultCore<V, T>::template Impl<true, true>(caller);
  }
#endif

  Result Retire() final {
    // Higher refcount can be alive SharedFutures or transient refs of this core's
    // callback dispatch, both forbid the move
    // Both arms construct a prvalue: with glvalue arms of mixed value category
    // the conditional would yield a const lvalue and the move arm would copy
    auto result = (this->GetRef() == 1) ? Result{std::move(this->Get())} : Result{std::as_const(this->Get())};
    this->DecRef();
    return result;
  }

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

extern template class SharedCore<void, DefaultTrait>;

template <typename V, typename T>
using SharedCorePtr = IntrusivePtr<SharedCore<V, T>>;

}  // namespace yaclib::detail
