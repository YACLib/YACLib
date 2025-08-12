#include <yaclib/algo/detail/base_core.hpp>

namespace yaclib::detail {

template <bool Shared>
[[nodiscard]] bool BaseCore::SetCallbackImpl(InlineCore& callback) noexcept {
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

template bool BaseCore::SetCallbackImpl<false>(InlineCore&) noexcept;
template bool BaseCore::SetCallbackImpl<true>(InlineCore&) noexcept;

[[nodiscard]] bool BaseCore::ResetImpl() noexcept {
  // Resetting a callback is not supported in shared cores
  auto expected = _callback.load(std::memory_order_relaxed);
  return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
}

template <bool SymmetricTransfer, bool Shared>
[[nodiscard]] Transfer<SymmetricTransfer> BaseCore::SetResultImpl() noexcept {
  const auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
  YACLIB_ASSERT(expected != kResult);
  if constexpr (Shared) {
    auto* head = reinterpret_cast<InlineCore*>(expected);
    while (head && head->next) {
      auto next = head->next;
      Loop(this, head);
      head = static_cast<InlineCore*>(next);
    }
    DecRef();
    if (head) {
      // If the refcount here is 2, the callback is the last one for this core
      // (no shared futures left), so the value may be moved
      Loop(this, head);
    }
    DecRef();
    DecRef();
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

template Transfer<false> BaseCore::SetResultImpl<false, false>() noexcept;
template Transfer<false> BaseCore::SetResultImpl<false, true>() noexcept;

#if YACLIB_SYMMETRIC_TRANSFER != 0
template Transfer<true> BaseCore::SetResultImpl<true, false>() noexcept;
template Transfer<true> BaseCore::SetResultImpl<true, true>() noexcept;
#endif

}  // namespace yaclib::detail
