#pragma once

#include <yaclib/coro/detail/promise_type.hpp>
#include <yaclib/lazy/task.hpp>

/**
 * TODO(mkornaukhov03) Add doxygen docs
 */
template <typename V, typename T, typename... Args>
struct yaclib_std::coroutine_traits<yaclib::Task<V, T>, Args...> final {
  using promise_type = yaclib::detail::PromiseType<V, T, true>;
};
