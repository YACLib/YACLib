#pragma once

#include <yaclib/config.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/coro.hpp>
#endif

namespace yaclib::detail {

class InlineCore : public Job {
 public:
  [[nodiscard]] virtual InlineCore* Here(InlineCore& /*caller*/) noexcept = 0;

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept {
    NextImpl(caller);
    return yaclib_std::noop_coroutine();
  }
#elif YACLIB_CORO != 0
  void Next(InlineCore& caller) noexcept {
    NextImpl(caller);
  }
#endif

 private:
#if YACLIB_CORO != 0
  YACLIB_INLINE void NextImpl(InlineCore& caller) noexcept {
    InlineCore* prev = &caller;
    InlineCore* next = this;
    do {
      auto* next_next = next->Here(*prev);
      prev = next;
      next = next_next;
    } while (next != nullptr);
  }
#endif
};

}  // namespace yaclib::detail
