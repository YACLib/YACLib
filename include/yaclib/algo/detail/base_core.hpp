#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/ref.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/coro.hpp>
#endif

#include <cstdint>
#include <limits>
#include <yaclib_std/atomic>

namespace yaclib::detail {

class BaseCore : public InlineCore {
 public:
  enum State : std::uintptr_t {
    kEmpty = std::uintptr_t{0},
    kResult = std::numeric_limits<std::uintptr_t>::max(),
  };

  void StoreCallback(InlineCore& callback) noexcept;

  [[nodiscard]] bool Empty() const noexcept;

  [[nodiscard]] bool SetCallback(InlineCore& callback) noexcept;

  [[nodiscard]] bool Reset() noexcept;

  // sometimes we know it will be last callback in cycle, so we want call it right now, instead of SetInline
  void CallInline(InlineCore& callback) noexcept;

  template <bool SymmetricTransfer>
#if YACLIB_SYMMETRIC_TRANSFER != 0
  using Transfer = std::conditional_t<SymmetricTransfer, yaclib_std::coroutine_handle<>, InlineCore*>;
#else
  using Transfer = InlineCore*;
#endif

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetInline(InlineCore& callback) noexcept;

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept;

#if YACLIB_CORO != 0                                                      // Compiler inline this call in tests
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Curr() noexcept {  // LCOV_EXCL_LINE
    YACLIB_PURE_VIRTUAL();                                                // LCOV_EXCL_LINE
    return {};                                                            // LCOV_EXCL_LINE
  }                                                                       // LCOV_EXCL_LINE
#endif

 protected:
  explicit BaseCore(State callback) noexcept;

  yaclib_std::atomic_uintptr_t _callback;

 public:
  IExecutorPtr _executor{NoRefTag{}, &MakeInline()};

  void MoveExecutorTo(BaseCore& callback) noexcept;
};

YACLIB_INLINE void Loop(InlineCore* prev, InlineCore* curr) noexcept {
  while (curr != nullptr) {
    auto* next = curr->Here(*prev);
    prev = curr;
    curr = next;
  }
}

}  // namespace yaclib::detail
