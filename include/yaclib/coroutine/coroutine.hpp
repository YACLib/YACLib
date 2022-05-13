#pragma once

#include <yaclib/config.hpp>

#ifdef __clang_major__
#  if __clang_major__ < 7
#    define YACLIB_SYMMETRIC_TRANSFER 0
#  else
#    define YACLIB_SYMMETRIC_TRANSFER 1
#  endif
#elif defined(_MSC_VER) && YACLIB_CORO == 1
#  define YACLIB_SYMMETRIC_TRANSFER 0
#else
#  define YACLIB_SYMMETRIC_TRANSFER 1
#endif

#if YACLIB_CORO == 2
#  include <coroutine>

namespace yaclib_std {

using std::coroutine_handle;
using std::coroutine_traits;
using std::suspend_always;
using std::suspend_never;

#  if YACLIB_SYMMETRIC_TRANSFER == 1
using std::noop_coroutine;
#  endif

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
#  if YACLIB_SYMMETRIC_TRANSFER == 1
using std::experimental::noop_coroutine;
#  endif
}  // namespace yaclib_std
#else
#  error "Don't have coroutine header"
#endif

#if YACLIB_SYMMETRIC_TRANSFER == 1
namespace yaclib_std {

using suspend_type = yaclib_std::coroutine_handle<>;

}  // namespace yaclib_std

#  define YACLIB_TRANSFER(handle)                                                                                      \
    return yaclib_std::suspend_type {                                                                                  \
      handle                                                                                                           \
    }
#  define YACLIB_RESUME(handle)                                                                                        \
    return yaclib_std::suspend_type {                                                                                  \
      handle                                                                                                           \
    }
#  define YACLIB_SUSPEND()                                                                                             \
    return yaclib_std::suspend_type {                                                                                  \
      yaclib_std::noop_coroutine()                                                                                     \
    }

#else
namespace yaclib_std {

using suspend_type = bool;

}  // namespace yaclib_std

#  define YACLIB_TRANSFER(handle)                                                                                      \
    handle.resume();                                                                                                   \
    return true
#  define YACLIB_RESUME(handle) return false
#  define YACLIB_SUSPEND() return true
#endif
