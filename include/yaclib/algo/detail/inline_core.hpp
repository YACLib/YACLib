#pragma once

#include <yaclib/config.hpp>
#include <yaclib/exe/job.hpp>
#include <yaclib/log.hpp>

#if YACLIB_SYMMETRIC_TRANSFER != 0
#  include <yaclib/coro/coro.hpp>
#endif

namespace yaclib::detail {

class InlineCore : public Job {
 public:
  [[nodiscard]] virtual InlineCore* Here(InlineCore& caller) noexcept = 0;

#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] virtual yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept = 0;
#endif
};

template <bool SymmetricTransfer>
YACLIB_INLINE auto Step([[maybe_unused]] InlineCore& caller, InlineCore& callback) noexcept {
#if YACLIB_SYMMETRIC_TRANSFER != 0
  if constexpr (SymmetricTransfer) {
    return callback.Next(caller);
  } else
#endif
  {
    return &callback;
  }
}

template <bool SymmetricTransfer>
YACLIB_INLINE auto Noop() noexcept {
#if YACLIB_SYMMETRIC_TRANSFER != 0
  if constexpr (SymmetricTransfer) {
    return yaclib_std::coroutine_handle<>{yaclib_std::noop_coroutine()};
  } else
#endif
  {
    return static_cast<InlineCore*>(nullptr);
  }
}

}  // namespace yaclib::detail
