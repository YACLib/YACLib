#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/coroutine/detail/suspend_condition.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/result.hpp>

#include <iostream>

namespace yaclib::detail {

struct ResumeDeleter {
  template <typename V, typename E>
  static void Delete(ResultCore<V, E>* res_core) noexcept {
    using promise_type = typename yaclib_std::coroutine_traits<Future<V, E>>::promise_type;

    assert(res_core);
    auto promise = static_cast<promise_type*>(res_core);

    assert(promise);
    auto coro_handle = yaclib_std::coroutine_handle<promise_type>::from_promise(*promise);

    assert(coro_handle);
    assert(coro_handle.done());
    coro_handle.destroy();
  }
};
}  // namespace yaclib::detail
