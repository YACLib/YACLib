#pragma once

#include <yaclib/coroutine/detail/on_awaiter.hpp>
#include <yaclib/executor/executor.hpp>

namespace yaclib {

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
YACLIB_INLINE detail::OnAwaiter On(IExecutor& e) noexcept {
  return detail::OnAwaiter{e};
}

}  // namespace yaclib
