#pragma once

#include <yaclib/config.hpp>
#include <yaclib/exe/job.hpp>

#include <cstdint>

#if YACLIB_SYMMETRIC_TRANSFER != 0
#  include <yaclib/coro/coro.hpp>
#endif

namespace yaclib::detail {

class InlineCore : public Job {
 public:
#if YACLIB_SYMMETRIC_TRANSFER != 0
  virtual yaclib_std::coroutine_handle<> Next() noexcept {
    Call();
    return yaclib_std::noop_coroutine();
  }
#endif

  virtual void Here(InlineCore& /*caller*/) noexcept {  // LCOV_EXCL_LINE  compiler remove this call from tests
    YACLIB_PURE_VIRTUAL();                              // LCOV_EXCL_LINE
  }                                                     // LCOV_EXCL_LINE
};

InlineCore& MakeEmpty();

}  // namespace yaclib::detail
