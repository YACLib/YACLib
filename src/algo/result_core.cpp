#include <yaclib/algo/detail/core.hpp>
#include <yaclib/algo/detail/result_core.hpp>

namespace yaclib::detail {
namespace {

class Drop final : public InlineCore {
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept final {
    caller.DecRef();
    return nullptr;
  }
};

static Drop kDropCore;

}  // namespace

template class ResultCore<void, StopError>;

InlineCore& MakeDrop() noexcept {
  return kDropCore;
}

}  // namespace yaclib::detail
