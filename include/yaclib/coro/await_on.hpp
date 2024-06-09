#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/coro.hpp>
#include <yaclib/coro/detail/await_on_awaiter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename... V, typename... E>
YACLIB_INLINE auto AwaitOn(IExecutor& e, FutureBase<V, E>&... fs) noexcept {
  return detail::AwaitOnAwaiter<sizeof...(fs) == 1>{e, UpCast<detail::BaseCore>(*fs.GetCore())...};
}

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename Iterator>
YACLIB_INLINE auto AwaitOn(IExecutor& e, Iterator begin, std::size_t count) noexcept
  -> std::enable_if_t<!is_future_base_v<Iterator>, detail::AwaitOnAwaiter<false>> {
  return detail::AwaitOnAwaiter<false>{e, begin, count};
}

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename BeginIt, typename EndIt>
YACLIB_INLINE auto AwaitOn(IExecutor& e, BeginIt begin, EndIt end) noexcept
  -> std::enable_if_t<!is_future_base_v<BeginIt>, detail::AwaitOnAwaiter<false>> {
  // We don't use std::distance because we want to alert the user to the fact that it can be expensive.
  // Maybe the user has the size of the range, otherwise it is suggested to call Await(begin, distance(begin, end))
  return AwaitOn(e, begin, static_cast<std::size_t>(end - begin));
}

}  // namespace yaclib
