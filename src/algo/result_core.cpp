#include <yaclib/algo/detail/result_core.hpp>

namespace yaclib::detail {
namespace {

class Empty final : public InlineCore {};

static Empty kEmptyCore;

class Drop final : public InlineCore {
  void Here(BaseCore& caller) noexcept final {
    caller.DecRef();
  }

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {
    Here(caller);
    return yaclib_std::noop_coroutine();
  }
#endif
};

static Drop kDropCore;

}  // namespace

template class ResultCore<void, StopError>;

InlineCore& MakeEmpty() noexcept {
  return kEmptyCore;
}

InlineCore& MakeDrop() noexcept {
  return kDropCore;
}

}  // namespace yaclib::detail
