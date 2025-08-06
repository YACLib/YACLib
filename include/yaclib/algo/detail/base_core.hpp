#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/inline.hpp>

#include <limits>
#include <yaclib_std/atomic>

namespace yaclib::detail {

class BaseCore : public InlineCore {
  friend class UniqueBaseHandle;
  friend class SharedBaseHandle;

 public:
  enum State : std::uintptr_t {
    kEmpty = std::uintptr_t{0},
    kResult = std::numeric_limits<std::uintptr_t>::max(),
  };

  bool Empty() const noexcept {
    auto callback = _callback.load(std::memory_order_acquire);
    YACLIB_DEBUG(callback != kEmpty && callback != kResult,
                 "That means we call it on already used future or on promise");
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
  explicit BaseCore(State state) noexcept : _callback(state) {
  }

  template <bool Shared>
  void StoreCallbackImpl(InlineCore& callback) noexcept {
    static_assert(!Shared, "StoreCallback is not supported in shared cores");
    _callback.store(reinterpret_cast<std::uintptr_t>(&callback), std::memory_order_relaxed);
  }

  template <bool Shared>
  [[nodiscard]] bool TryAddCallbackImpl(InlineCore& callback) noexcept {
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(&callback) != kEmpty);
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(&callback) != kResult);
    if constexpr (Shared) {
      auto next = _callback.load(std::memory_order_acquire);
      do {
        if (next == kResult) {
          return false;
        }
        callback.next = reinterpret_cast<InlineCore*>(next);
      } while (!_callback.compare_exchange_weak(next, reinterpret_cast<std::uintptr_t>(&callback),
                                                std::memory_order_release, std::memory_order_acquire));
      return true;
    } else {
      std::uintptr_t expected = kEmpty;
      return _callback.load(std::memory_order_acquire) == expected &&
             _callback.compare_exchange_strong(expected, reinterpret_cast<std::uintptr_t>(&callback),
                                               std::memory_order_release, std::memory_order_acquire);
    }
  }

  template <bool Shared>
  [[nodiscard]] bool ResetImpl() noexcept {
    static_assert(!Shared, "Resetting a callback is not supported in shared cores");
    auto expected = _callback.load(std::memory_order_relaxed);
    return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
  }

  template <bool Shared>
  void CallInlineImpl(InlineCore& callback) noexcept {
    if (!TryAddCallbackImpl<Shared>(callback)) {
      auto* next = callback.Here(*this);
      YACLIB_ASSERT(next == nullptr);
    }
  }

  template <bool SymmetricTransfer, bool Shared>
  [[nodiscard]] Transfer<SymmetricTransfer> SetInlineImpl(InlineCore& callback) noexcept {
    if (!TryAddCallbackImpl<Shared>(callback)) {
      return Step<SymmetricTransfer>(*this, callback);
    }
    return Noop<SymmetricTransfer>();
  }

  template <bool SymmetricTransfer, bool Shared>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResultImpl() noexcept {
    const auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
    YACLIB_ASSERT(expected != kResult);
    if constexpr (Shared) {
      auto* const head = reinterpret_cast<InlineCore*>(expected);
      // Need to call it even when null to release ownership
      FulfillQueue(head);
      return Noop<SymmetricTransfer>();
    } else {
      if (expected != kEmpty) {
        auto* const callback = reinterpret_cast<InlineCore*>(expected);
        return Step<SymmetricTransfer>(*this, *callback);
      } else {
        return Noop<SymmetricTransfer>();
      }
    }
  }

  void FulfillQueue(InlineCore* head) noexcept {
    while (head) {
      auto next = head->next;
      Loop(this, head);
      head = static_cast<InlineCore*>(next);
    }
    DecRef();
    DecRef();
  }

  yaclib_std::atomic_uintptr_t _callback;
};

class UniqueBaseHandle {
 public:
  explicit UniqueBaseHandle(BaseCore& core) : _core(core) {
  }

  bool TryAddCallback(InlineCore& callback) noexcept {
    return _core.TryAddCallbackImpl<false>(callback);
  }

  bool Reset() noexcept {
    return _core.ResetImpl<false>();
  }

  void StoreCallback(InlineCore& callback) noexcept {
    return _core.StoreCallbackImpl<false>(callback);
  }

  template <bool SymmetricTransfer>
  [[nodiscard]] Transfer<SymmetricTransfer> SetResult() noexcept {
    return _core.SetResultImpl<SymmetricTransfer, false>();
  }

  BaseCore& _core;
};

class SharedBaseHandle {
 public:
  explicit SharedBaseHandle(BaseCore& core) : _core(core) {
  }

  bool TryAddCallback(InlineCore& callback) noexcept {
    return _core.TryAddCallbackImpl<true>(callback);
  }

  BaseCore& _core;
};

}  // namespace yaclib::detail
