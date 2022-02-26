#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/coroutine/detail/resume_deleter.hpp>

template <typename V, typename E, typename... Args>
struct yaclib_std::coroutine_traits<yaclib::Future<V, E>, Args...> {
  using promise_type = yaclib::detail::PromiseType<V, E>;
};
