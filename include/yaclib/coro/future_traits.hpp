#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename V, typename E, typename... Args>
struct yaclib_std::coroutine_traits<yaclib::Future<V, E>, Args...> {
  using promise_type = yaclib::detail::PromiseType<V, E>;
};
