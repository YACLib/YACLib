#pragma once

#include <yaclib/async/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/detail/suspend_condition.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib::detail {

struct CoroutineDeleter {
  template <typename V, typename E>
  static void Delete(ResultCore<V, E>* res_core) noexcept {
    using PromiseType = typename yaclib_std::coroutine_traits<Future<V, E>>::promise_type;
    assert(res_core);
    auto promise = static_cast<PromiseType*>(res_core);
    assert(promise);
    auto handle = yaclib_std::coroutine_handle<PromiseType>::from_promise(*promise);
    assert(handle);
    handle.destroy();
  }
};
}  // namespace yaclib::detail
