#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/async/detail/inline_core.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/nope_counter.hpp>

namespace yaclib::detail {

void PCore::Call() noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

void PCore::Drop() noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

void PCore::Here(PCore&, PCore::State) noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

static NopeCounter<IRef> kEmptyRef;

void CCore::SetCall(CCore& callback) noexcept {
  callback._caller = this;  // move ownership
  if (!SetCallback(callback, kCall)) {
    Submit(callback);
  }
}

void CCore::SetHere(PCore& callback, State state) noexcept {
  YACLIB_ASSERT(state == kHereCall || state == kHereWrap);
  if (!SetCallback(callback, state)) {
    Submit(callback, state);
  }
}

bool CCore::SetWait(IRef& callback, State state) noexcept {
  // YACLIB_ASSERT(state == kWaitNope);
  return SetCallback(callback, state);
}

void CCore::SetWait(State state) noexcept {
  YACLIB_ASSERT(state == kWaitDrop || state == kWaitStop);
  if (!SetCallback(const_cast<NopeCounter<IRef>&>(kEmptyRef), state)) {
    DecRef();
  }
}

bool CCore::ResetWait() noexcept {
  std::uintptr_t expected = _callback.load(std::memory_order_relaxed);
  return expected != kResult && _callback.compare_exchange_strong(expected, kEmpty, std::memory_order_relaxed);
}

bool CCore::Empty() const noexcept {
  auto callback = _callback.load(std::memory_order_acquire);
  YACLIB_ASSERT(callback == kEmpty || callback == kResult);
  return callback == kEmpty;
}

bool CCore::Alive() const noexcept {
  const State callback{kMask & _callback.load(std::memory_order_acquire)};
  YACLIB_DEBUG(callback == kResult, "No needed to check Alive after store kResult or from Future side");
  return callback != kWaitStop;
}

void CCore::SetCaller(CCore& caller) noexcept {
  _caller = &caller;
}

IExecutorPtr& CCore::GetExecutor() noexcept {
  return _executor;
}

void CCore::SetExecutor(IExecutorPtr executor) noexcept {
  _executor = std::move(executor);
}

CCore::CCore(State callback) noexcept : _callback{callback}, _caller{&kEmptyRef} {
}

void CCore::SetResult() noexcept {
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
      auto* callback = reinterpret_cast<IRef*>(expected & ~kMask);
      YACLIB_ASSERT(callback != nullptr);
      Submit(static_cast<CCore&>(*callback));
    } break;
    case kHereWrap:
      [[fallthrough]];
    case kHereCall: {
      auto* callback = reinterpret_cast<IRef*>(expected & ~kMask);
      YACLIB_ASSERT(callback != nullptr);
      Submit(static_cast<CCore&>(*callback), state);
    } break;
    case kWaitStop:
      [[fallthrough]];
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

bool CCore::SetCallback(IRef& callback, State state) noexcept {
  std::uintptr_t expected = kEmpty;
  return _callback.load(std::memory_order_acquire) == expected &&
         _callback.compare_exchange_strong(expected, state | reinterpret_cast<std::uintptr_t>(&callback),
                                           std::memory_order_release, std::memory_order_acquire);
}

void CCore::Submit(CCore& callback) noexcept {
  YACLIB_ASSERT(_caller == &kEmptyRef);
  YACLIB_ASSERT(_executor != nullptr);
  auto executor = std::move(_executor);
  executor->Submit(callback);
}

void CCore::Submit(PCore& callback, State state) noexcept {
  YACLIB_ASSERT(state == kHereCall || state == kHereWrap);
  YACLIB_ASSERT(_caller == &kEmptyRef);
  callback.Here(*this, state);
  DecRef();
}

#ifdef YACLIB_LOG_DEBUG
CCore::~CCore() noexcept {
  YACLIB_ASSERT(_caller == &kEmptyRef);
}
#endif

}  // namespace yaclib::detail
