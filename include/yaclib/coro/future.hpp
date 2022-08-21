#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/lazy/task.hpp>

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename V, typename E, typename... Args>
struct yaclib_std::coroutine_traits<yaclib::Future<V, E>, Args...> final {
  using promise_type = yaclib::detail::PromiseType<V, E, false>;
};
