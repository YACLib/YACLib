#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coroutine.hpp>
#include <yaclib/coro/detail/await_awaiter.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename... V, typename... E>
YACLIB_INLINE detail::AwaitAwaiter Await(FutureBase<V, E>&... fs) noexcept {
  return detail::AwaitAwaiter{static_cast<detail::BaseCore&>(*fs.GetCore())...};
}

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename Iterator>
YACLIB_INLINE std::enable_if_t<!is_future_base_v<Iterator>, detail::AwaitAwaiter> Await(Iterator begin,
                                                                                        Iterator end) noexcept {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return detail::AwaitAwaiter{begin, end - begin};
}

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename Iterator>
YACLIB_INLINE std::enable_if_t<!is_future_base_v<Iterator>, detail::AwaitAwaiter> Await(Iterator begin,
                                                                                        std::size_t count) noexcept {
  return detail::AwaitAwaiter{begin, count};
}

}  // namespace yaclib
