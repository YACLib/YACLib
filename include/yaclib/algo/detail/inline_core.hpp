#pragma once

#include <yaclib/config.hpp>
#include <yaclib/exe/job.hpp>

#include <cstdint>

#if YACLIB_CORO != 0
#  include <yaclib/coro/coro.hpp>
#endif

namespace yaclib::detail {

class BaseCore;

class InlineCore : public Job {
 public:
  // Compiler inline this call in tests
  virtual void Here(BaseCore& /*caller*/) noexcept {  // LCOV_EXCL_LINE
    YACLIB_PURE_VIRTUAL();                            // LCOV_EXCL_LINE
  }                                                   // LCOV_EXCL_LINE

#if YACLIB_FINAL_SUSPEND_TRANSFER != 0
  // Compiler inline this call in tests
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Next(BaseCore& caller) noexcept {  // LCOV_EXCL_LINE
    YACLIB_PURE_VIRTUAL();                                                                // LCOV_EXCL_LINE
    return {};                                                                            // LCOV_EXCL_LINE
  }                                                                                       // LCOV_EXCL_LINE
#endif
};

InlineCore& MakeEmpty() noexcept;
InlineCore& MakeDrop() noexcept;

}  // namespace yaclib::detail
