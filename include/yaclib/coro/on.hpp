#pragma once

#include <yaclib/coro/detail/on_awaiter.hpp>
#include <yaclib/exe/executor.hpp>

namespace yaclib {

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename E>
YACLIB_INLINE detail::OnAwaiter<E> On(E& e) noexcept {
  return detail::OnAwaiter<E>{e};
}

}  // namespace yaclib
