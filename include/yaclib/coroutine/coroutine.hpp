#pragma once

#include <yaclib/config.hpp>

#if YACLIB_CORO == 2
#  include <coroutine>

namespace yaclib_std {

using std::coroutine_handle;
using std::coroutine_traits;
using std::noop_coroutine;
using std::suspend_always;
using std::suspend_never;

}  // namespace yaclib_std
#elif YACLIB_CORO == 1
#  include <experimental/coroutine>

namespace yaclib_std {

using std::experimental::coroutine_handle;
using std::experimental::coroutine_traits;
using std::experimental::noop_coroutine;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

}  // namespace yaclib_std
#else
#  error "Don't have coroutine header"
#endif
