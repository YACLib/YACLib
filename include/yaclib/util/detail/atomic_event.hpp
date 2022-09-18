#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>

namespace yaclib::detail {

class AtomicEvent {
 public:
  using Token = Unit;

  static Token Make() noexcept;

  void Wait(Token) noexcept;

  void Set() noexcept;

  void Reset() noexcept;

#if !defined(_LIBCPP_VERSION) || defined(__linux__) || (defined(_AIX) && !defined(__64BIT__))
  using wait_t = std::int32_t;
#else
  using wait_t = std::int64_t;
#endif
 private:
  yaclib_std::atomic<wait_t> _state = 0;
};

}  // namespace yaclib::detail
