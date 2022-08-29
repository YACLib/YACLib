#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

class Empty final : public InlineCore {};

static Empty kEmptyCore;

InlineCore& MakeEmpty() {
  return kEmptyCore;
}

void BaseCore::SetInline(InlineCore& callback) noexcept {
  if (!SetCallback(callback, BaseCore::kInline)) {
    callback.Here(*this);
    DecRef();
  }
}

void BaseCore::SetCall(BaseCore& callback) noexcept {
  callback._caller = this;  // move ownership
  if (!SetCallback(callback, kCall)) {
    Submit(callback);
  }
}

bool BaseCore::Reset() noexcept {
  std::uint64_t expected = _callback.load(std::memory_order_relaxed);
  return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
}

bool BaseCore::Empty() const noexcept {
  auto callback = _callback.load(std::memory_order_acquire);
  YACLIB_DEBUG(callback != kEmpty && callback != kResult, "That means we call it on already used future or on promise");
  return callback == kEmpty;
}

BaseCore::BaseCore(State callback) noexcept : _callback{callback}, _caller{&kEmptyCore} {
}

template <bool SymmetricTransfer>
BaseCore::ReturnT<SymmetricTransfer> BaseCore::SetResult() noexcept {
  auto* caller = _caller;
  YACLIB_ASSERT(std::exchange(_caller, &kEmptyCore) != nullptr);
  caller->DecRef();
  auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
  const State state{expected & kMask};
  auto* const callback = reinterpret_cast<InlineCore*>(expected & ~kMask);
  YACLIB_ASSERT(state == kEmpty || callback != nullptr);
  switch (state) {
    case kWaitDrop:
      DecRef();
      [[fallthrough]];
    case kWaitNope:
#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
      if constexpr (SymmetricTransfer) {
        return callback->Next();
      } else
#endif
      {
        callback->Call();
      }
      break;
    case kInline:
      callback->Here(*this);
      DecRef();
      break;
    case kCall:
      YACLIB_ASSERT(static_cast<detail::BaseCore*>(callback)->_caller == this);
      Submit(*callback);
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
  // TODO with atomic_ref here can be non-atomic store
  _callback.store(state | reinterpret_cast<std::uint64_t>(&callback), std::memory_order_relaxed);
}

void BaseCore::Submit(Job& job) noexcept {
  YACLIB_ASSERT(_executor != nullptr);
  auto executor = std::move(_executor);
  executor->Submit(job);
}

}  // namespace yaclib::detail
