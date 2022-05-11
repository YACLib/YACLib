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

// TODO(mkornaukhov03) figure out more convinient way how to check the possibility of Symmetric Transfer
using SuspendType = std::coroutine_handle<>;
#  define YACLIB_SUSPEND() return yaclib_std::noop_coroutine();
#  define YACLIB_RESUME(handle) return handle;

}  // namespace yaclib_std
#elif YACLIB_CORO == 1
#  include <experimental/coroutine>

namespace yaclib_std {

using std::experimental::coroutine_handle;
using std::experimental::coroutine_traits;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

// TODO(mkornaukhov03) figure out more convinient way how to check the possibility of Symmetric Transfer
// Now it has a problem: doesn't use Symmetric Transfer even if possible
using SuspendType = bool;
#  define YACLIB_SUSPEND() return true;
#  define YACLIB_RESUME(handle)                                                                                        \
    handle.resume();                                                                                                   \
    return true;

}  // namespace yaclib_std
#else
#  error "Don't have coroutine header"
#endif
