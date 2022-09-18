#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

void BaseCore::SetInline(InlineCore& callback) noexcept {
  if (!SetCallback(callback, BaseCore::kInline)) {
    callback.Here(*this);
  }
}

void BaseCore::SetCall(BaseCore& callback) noexcept {
  if (!SetCallback(callback, kCall)) {
    Submit(callback);
  }
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
  auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
  const State state{expected & kMask};
  auto* const callback = reinterpret_cast<InlineCore*>(expected & ~kMask);
  YACLIB_ASSERT(state == kEmpty || callback != nullptr);
  switch (state) {
    case kInline:
#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
      if constexpr (SymmetricTransfer) {
        return callback->Next(*this);
      } else
#endif
      {
        callback->Here(*this);
      }
      break;
    case kCall:
      Submit(static_cast<BaseCore&>(*callback));
      [[fallthrough]];
    default:
      break;
  }
#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  if constexpr (SymmetricTransfer) {
    return yaclib_std::noop_coroutine();
  }
#endif
}

template BaseCore::ReturnT<false> BaseCore::SetResult<false>() noexcept;

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
template BaseCore::ReturnT<true> BaseCore::SetResult<true>() noexcept;
#endif

bool BaseCore::SetCallback(InlineCore& callback, State state) noexcept {
  std::uint64_t expected = kEmpty;
  return _callback.load(std::memory_order_acquire) == expected &&
         _callback.compare_exchange_strong(expected, state | reinterpret_cast<std::uint64_t>(&callback),
                                           std::memory_order_release, std::memory_order_acquire);
}

void BaseCore::StoreCallback(InlineCore& callback, State state) noexcept {
  // TODO(MBkkt) with atomic_ref here can be non-atomic store
  _callback.store(state | reinterpret_cast<std::uint64_t>(&callback), std::memory_order_relaxed);
}

void BaseCore::Submit(BaseCore& callback) noexcept {
  if (!callback._executor) {
    YACLIB_ASSERT(_executor != nullptr);
    callback._executor = std::move(_executor);
  }
  callback._executor->Submit(callback);
}

}  // namespace yaclib::detail
