#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/nope_counter.hpp>

namespace yaclib::detail {

static NopeCounter<IRef> kEmptyRef;

void BaseCore::SetCall(BaseCore& callback) noexcept {
  callback._caller = this;  // move ownership
  if (!SetCallback(&callback, kCall)) {
    Submit(callback);
  }
}

void BaseCore::SetHere(InlineCore& callback, State state) noexcept {
  YACLIB_ASSERT(state == kHereCall || state == kHereWrap);
  if (!SetCallback(&callback, state)) {
    Submit(callback, state);
  }
}

bool BaseCore::SetWait(IRef& callback, State state) noexcept {
  // YACLIB_ASSERT(state == kWaitNope);
  return SetCallback(&callback, state);
}

void BaseCore::SetWait(State state) noexcept {
  YACLIB_ASSERT(state == kWaitDrop);
  if (!SetCallback(&static_cast<IRef&>(kEmptyRef), state)) {
    DecRef();
  }
}

bool BaseCore::ResetWait() noexcept {
  std::uint64_t expected = _callback.load(std::memory_order_relaxed);
  return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
}

bool BaseCore::Empty() const noexcept {
  auto callback = _callback.load(std::memory_order_acquire);
  YACLIB_ASSERT(callback == kEmpty || callback == kResult);
  return callback == kEmpty;
}

BaseCore::BaseCore(State callback) noexcept : _callback{callback}, _caller{&kEmptyRef} {
}

void BaseCore::SetResult() noexcept {
  auto* caller = _caller;
  YACLIB_ASSERT(std::exchange(_caller, &kEmptyRef) != nullptr);
  caller->DecRef();
  // auto expected = _callback.load(std::memory_order_acquire);
  // if (expected == kEmpty || (expected & kMask) == kWaitNope) {
  //   _callback.compare_exchange_strong(expected, kResult, std::memory_order_release, std::memory_order_acquire);
  // }
  auto expected = _callback.exchange(kResult, std::memory_order_acq_rel);
  const State state{expected & kMask};
  switch (state) {
    case kCall: {
      auto* callback = reinterpret_cast<BaseCore*>(expected & ~kMask);
      YACLIB_ASSERT(callback != nullptr);
      Submit(*callback);
    } break;
    case kHereWrap:
      [[fallthrough]];
    case kHereCall: {
      auto* callback = reinterpret_cast<InlineCore*>(expected & ~kMask);
      YACLIB_ASSERT(callback != nullptr);
      Submit(*callback, state);
    } break;
    case kWaitDrop:
      DecRef();
      [[fallthrough]];
    case kWaitNope: {
      auto* callback = reinterpret_cast<IRef*>(expected & ~kMask);
      YACLIB_ASSERT(callback != nullptr);
      callback->DecRef();
      [[fallthrough]];
    }
    default:
      break;
  }
}

bool BaseCore::SetCallback(void* callback, State state) noexcept {
  std::uint64_t expected = kEmpty;
  return _callback.load(std::memory_order_acquire) == expected &&
         _callback.compare_exchange_strong(expected, state | reinterpret_cast<std::uint64_t>(callback),
                                           std::memory_order_release, std::memory_order_acquire);
}

void BaseCore::StoreCallback(void* callback, State state) noexcept {
  // TODO with atomic_ref can be non atomic store
  _callback.store(state | reinterpret_cast<std::uint64_t>(callback), std::memory_order_relaxed);
}

void BaseCore::Submit(BaseCore& callback) noexcept {
  YACLIB_ASSERT(_caller == &kEmptyRef);
  YACLIB_ASSERT(_executor != nullptr);
  auto executor = std::move(_executor);
  executor->Submit(callback);
}

void BaseCore::Submit(InlineCore& callback, State state) noexcept {
  YACLIB_ASSERT(state == kHereCall || state == kHereWrap);
  YACLIB_ASSERT(_caller == &kEmptyRef);
  callback.Here(*this, state);
  DecRef();
}

#ifdef YACLIB_LOG_DEBUG
BaseCore::~BaseCore() noexcept {
  YACLIB_ASSERT(_caller == &kEmptyRef);
}
#endif

}  // namespace yaclib::detail
