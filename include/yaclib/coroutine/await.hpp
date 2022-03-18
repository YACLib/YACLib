#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/future_awaiter.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

template <typename... V, typename... E>
detail::FutureAwaiter Await(Future<V, E>&... fs) {
  return detail::FutureAwaiter(fs...);
}

template <typename Iterator>
std::enable_if_t<!is_future_v<Iterator>, detail::FutureAwaiter> Await(Iterator begin, Iterator end) {
  return detail::FutureAwaiter(begin, begin, end);
}

template <typename Iterator>
std::enable_if_t<!is_future_v<Iterator>, detail::FutureAwaiter> Await(Iterator begin, std::size_t count) {
  return detail::FutureAwaiter(begin, std::size_t{0}, count);
}

}  // namespace yaclib
