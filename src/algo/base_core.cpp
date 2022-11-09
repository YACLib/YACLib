#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

InlineCore* BaseCore::SetInline(InlineCore& callback) noexcept {
  if (!SetCallback(callback)) {
    return callback.Here(*this);
  }
  return nullptr;
}

bool BaseCore::Reset() noexcept {
  auto expected = _callback.load(std::memory_order_relaxed);
  return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
}

bool BaseCore::Empty() const noexcept {
  auto callback = _callback.load(std::memory_order_acquire);
  YACLIB_DEBUG(callback != kEmpty && callback != kResult, "That means we call it on already used future or on promise");
  return callback == kEmpty;
}

BaseCore::BaseCore(State callback) noexcept : _callback{callback} {
}

template <bool SymmetricTransfer>
BaseCore::ReturnT<SymmetricTransfer> BaseCore::SetResult() noexcept {
  const auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
  if (expected != kEmpty) {
    YACLIB_ASSERT(expected != kResult);
    auto* const callback = reinterpret_cast<InlineCore*>(expected);
#if YACLIB_SYMMETRIC_TRANSFER != 0
    if constexpr (SymmetricTransfer) {
      return callback->Next(*this);
    } else
#endif
    {
      return callback;
    }
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  if constexpr (SymmetricTransfer) {
    return yaclib_std::noop_coroutine();
  } else
#endif
  {
    return nullptr;
  }
}

template BaseCore::ReturnT<false> BaseCore::SetResult<false>() noexcept;

#if YACLIB_SYMMETRIC_TRANSFER != 0
template BaseCore::ReturnT<true> BaseCore::SetResult<true>() noexcept;
#endif

bool BaseCore::SetCallback(InlineCore& callback) noexcept {
  YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(&callback) != kEmpty);
  YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(&callback) != kResult);
  std::uintptr_t expected = kEmpty;
  return _callback.load(std::memory_order_acquire) == expected &&
         _callback.compare_exchange_strong(expected, reinterpret_cast<std::uintptr_t>(&callback),
                                           std::memory_order_release, std::memory_order_acquire);
}

void BaseCore::StoreCallback(InlineCore& callback) noexcept {
  // TODO(MBkkt) with atomic_ref here can be non-atomic store
  _callback.store(reinterpret_cast<std::uintptr_t>(&callback), std::memory_order_relaxed);
}

void BaseCore::MoveExecutorTo(BaseCore& callback) noexcept {
  if (!callback._executor) {
    YACLIB_ASSERT(_executor != nullptr);
    callback._executor = std::move(_executor);
  }
}

}  // namespace yaclib::detail
