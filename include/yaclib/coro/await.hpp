#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename... V, typename... E>
YACLIB_INLINE auto Await(FutureBase<V, E>&... fs) noexcept {
  return detail::AwaitAwaiter<sizeof...(fs) == 1>{static_cast<detail::BaseCore&>(*fs.GetCore())...};
}

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename Iterator>
YACLIB_INLINE auto Await(Iterator begin, std::size_t count) noexcept
  -> std::enable_if_t<!is_future_base_v<Iterator>, detail::AwaitAwaiter<false>> {
  return detail::AwaitAwaiter<false>{begin, count};
}

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename Iterator>
YACLIB_INLINE auto Await(Iterator begin, Iterator end) noexcept
  -> std::enable_if_t<!is_future_base_v<Iterator>, detail::AwaitAwaiter<false>> {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return Await(begin, end - begin);
}

}  // namespace yaclib
