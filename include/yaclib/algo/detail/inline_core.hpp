#pragma once

#include <yaclib/config.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
#  include <yaclib/coro/coro.hpp>
#endif

namespace yaclib::detail {

class BaseCore;

#if YACLIB_NEXT_IMPL != 0 && YACLIB_FINAL_SUSPEND_TRANSFER != 0
#  define DEFAULT_NEXT_IMPL                                                                                            \
    [[nodiscard]] yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept final {                               \
      Here(caller);                                                                                                    \
      return yaclib_std::noop_coroutine();                                                                             \
    }
#else
#  define DEFAULT_NEXT_IMPL
#endif

class InlineCore : public Job {
 public:
#if YACLIB_NEXT_IMPL != 0 && YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Next(BaseCore& /*caller*/) noexcept = 0;
#elif YACLIB_FINAL_SUSPEND_TRANSFER != 0
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept {
    Here(caller);
    return yaclib_std::noop_coroutine();
  }
#endif
  virtual void Here(BaseCore& /*caller*/) noexcept = 0;
};

}  // namespace yaclib::detail
