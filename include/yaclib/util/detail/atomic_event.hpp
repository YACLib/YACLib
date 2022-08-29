#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>

namespace yaclib::detail {

class /*alignas(kCacheLineSize)*/ AtomicEvent {
 public:
  using Token = Unit;

  static Token Make() noexcept;

  void Wait(Token) noexcept;

  void Set() noexcept;

  void Reset() noexcept;

 private:
  yaclib_std::atomic_int32_t _state = 0;
};

}  // namespace yaclib::detail
