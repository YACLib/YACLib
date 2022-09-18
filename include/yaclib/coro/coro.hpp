#pragma once

#include <yaclib/config.hpp>

#if YACLIB_CORO == 2
#  include <coroutine>

namespace yaclib_std {

using std::coroutine_handle;
using std::coroutine_traits;
using std::suspend_always;
using std::suspend_never;

#  if YACLIB_SYMMETRIC_TRANSFER != 0
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

#  if YACLIB_SYMMETRIC_TRANSFER != 0
using std::experimental::noop_coroutine;
#  endif

}  // namespace yaclib_std
#else
#  error "Don't have coroutine header"
#endif

#if YACLIB_SYMMETRIC_TRANSFER != 0
#  define YACLIB_TRANSFER(handle)                                                                                      \
    return yaclib_std::coroutine_handle<> {                                                                            \
      handle                                                                                                           \
    }
#  define YACLIB_RESUME(handle) YACLIB_TRANSFER(handle)
#  define YACLIB_SUSPEND() YACLIB_TRANSFER(yaclib_std::noop_coroutine())
#else
namespace yaclib_std {

constexpr yaclib_std::coroutine_handle<> noop_coroutine() noexcept {
  return {};
}

}  // namespace yaclib_std
#  define YACLIB_TRANSFER(handle)                                                                                      \
    handle.resume();                                                                                                   \
    return true
#  define YACLIB_RESUME(handle) return false
#  define YACLIB_SUSPEND() return true
#endif
