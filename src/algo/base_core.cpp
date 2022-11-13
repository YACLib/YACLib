#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

void BaseCore::StoreCallback(InlineCore& callback) noexcept {
  // TODO(MBkkt) with atomic_ref here can be non-atomic store
  _callback.store(reinterpret_cast<std::uintptr_t>(&callback), std::memory_order_relaxed);
}

bool BaseCore::Empty() const noexcept {
  auto callback = _callback.load(std::memory_order_acquire);
  YACLIB_DEBUG(callback != kEmpty && callback != kResult, "That means we call it on already used future or on promise");
  return callback == kEmpty;
}

bool BaseCore::SetCallback(InlineCore& callback) noexcept {
  YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(&callback) != kEmpty);
  YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(&callback) != kResult);
  std::uintptr_t expected = kEmpty;
  return _callback.load(std::memory_order_acquire) == expected &&
         _callback.compare_exchange_strong(expected, reinterpret_cast<std::uintptr_t>(&callback),
                                           std::memory_order_release, std::memory_order_acquire);
}

bool BaseCore::Reset() noexcept {
  auto expected = _callback.load(std::memory_order_relaxed);
  return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
}

void BaseCore::CallInline(InlineCore& callback) noexcept {
  if (!SetCallback(callback)) {
    auto* next = callback.Here(*this);
    YACLIB_ASSERT(next == nullptr);
  }
}

template <bool SymmetricTransfer>
BaseCore::Transfer<SymmetricTransfer> BaseCore::SetInline(InlineCore& callback) noexcept {
  if (!SetCallback(callback)) {
    return Step<SymmetricTransfer>(*this, callback);
  }
  return Noop<SymmetricTransfer>();
}

template <bool SymmetricTransfer>
BaseCore::Transfer<SymmetricTransfer> BaseCore::SetResult() noexcept {
  const auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
  if (expected != kEmpty) {
    YACLIB_ASSERT(expected != kResult);
    auto* const callback = reinterpret_cast<InlineCore*>(expected);
    YACLIB_ASSERT(callback != nullptr);
    return Step<SymmetricTransfer>(*this, *callback);
  }
  return Noop<SymmetricTransfer>();
}

BaseCore::BaseCore(BaseCore::State callback) noexcept : _callback{callback} {
}

void BaseCore::MoveExecutorTo(BaseCore& callback) noexcept {
  if (!callback._executor) {
    YACLIB_ASSERT(_executor != nullptr);
    callback._executor = std::move(_executor);
  }
}

template BaseCore::Transfer<false> BaseCore::SetInline<false>(InlineCore& caller) noexcept;
#if YACLIB_SYMMETRIC_TRANSFER != 0
template BaseCore::Transfer<true> BaseCore::SetInline<true>(InlineCore& caller) noexcept;
#endif

template BaseCore::Transfer<false> BaseCore::SetResult<false>() noexcept;
#if YACLIB_SYMMETRIC_TRANSFER != 0
template BaseCore::Transfer<true> BaseCore::SetResult<true>() noexcept;
#endif

}  // namespace yaclib::detail
