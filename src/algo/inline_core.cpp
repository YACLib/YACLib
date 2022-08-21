#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/log.hpp>

namespace yaclib::detail {

void InlineCore::Call() noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

void InlineCore::Drop() noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

void InlineCore::Here(InlineCore&, InlineCore::State) noexcept {
  YACLIB_DEBUG(true, "Pure virtual call");
}

}  // namespace yaclib::detail
