#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/coro.hpp>
#endif

#include <limits>
#include <yaclib_std/atomic>

namespace yaclib::detail {

class BaseCore : public InlineCore {
  friend struct UniqueHandle;
  friend struct SharedHandle;

 public:
  enum State : std::uintptr_t {
    kEmpty = std::uintptr_t{0},
    kResult = std::numeric_limits<std::uintptr_t>::max(),
  };

  bool Empty() const noexcept {
    auto callback = _callback.load(std::memory_order_acquire);
    return callback == kEmpty;
  }

  void MoveExecutorTo(BaseCore& callback) noexcept {
    if (!callback._executor) {
      YACLIB_ASSERT(_executor != nullptr);
      callback._executor = std::move(_executor);
    }
  }

#if YACLIB_CORO != 0                                                      // Compiler inline this call in tests
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Curr() noexcept {  // LCOV_EXCL_LINE
    YACLIB_PURE_VIRTUAL();                                                // LCOV_EXCL_LINE
    return {};                                                            // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE
#endif

  IExecutorPtr _executor{NoRefTag{}, &MakeInline()};

 protected:
  explicit BaseCore(State state) noexcept : _callback{state} {
  }

  void StoreCallbackImpl(InlineCore& callback) noexcept {
    this->_callback.store(reinterpret_cast<std::uintptr_t>(&callback), std::memory_order_relaxed);
  }

  template <bool Shared>
  [[nodiscard]] bool SetCallbackImpl(InlineCore& callback) noexcept;

  [[nodiscard]] bool ResetImpl() noexcept;

  template <bool SymmetricTransfer, bool Shared>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResultImpl() noexcept;

  yaclib_std::atomic_uintptr_t _callback;
};

struct UniqueHandle {
  void StoreCallback(InlineCore& callback) noexcept {
    return core.StoreCallbackImpl(callback);
  }

  bool SetCallback(InlineCore& callback) noexcept {
    return core.SetCallbackImpl<false>(callback);
  }

  bool Reset() noexcept {
    return core.ResetImpl();
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept {
    return core.SetResultImpl<SymmetricTransfer, false>();
  }

  BaseCore& core;
};

struct SharedHandle {
  bool SetCallback(InlineCore& callback) noexcept {
    return core.SetCallbackImpl<true>(callback);
  }

  BaseCore& core;
};

}  // namespace yaclib::detail
