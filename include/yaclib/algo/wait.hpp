#pragma once

#include <yaclib/algo/detail/wait.hpp>

namespace yaclib {

/**
 * Wait until \ref Ready becomes true
 *
 * \param fs one or more futures to wait
 */
template <typename... Futures>
void Wait(Futures&&... fs) {
  detail::Wait(detail::NoTimeoutTag{}, static_cast<detail::BaseCore&>(*fs._core)...);
}

}  // namespace yaclib
