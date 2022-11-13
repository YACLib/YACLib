#include <yaclib/algo/detail/core.hpp>
#include <yaclib/algo/detail/result_core.hpp>

namespace yaclib::detail {
namespace {

class Drop final : public InlineCore {
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    caller.DecRef();
    return Noop<SymmetricTransfer>();
  }
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept final {
    return Impl<true>(caller);
  }
#endif
};

static Drop kDropCore;

}  // namespace

template class ResultCore<void, StopError>;

InlineCore& MakeDrop() noexcept {
  return kDropCore;
}

}  // namespace yaclib::detail
