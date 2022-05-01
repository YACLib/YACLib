#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/ref.hpp>

#include <yaclib_std/atomic>

namespace yaclib::detail {

class /*alignas(kCacheLineSize)*/ AtomicEvent : public IRef {
 public:
  using Token = Unit;

  static Token Make() noexcept;

  void Wait(Token) noexcept;

  void SetOne() noexcept;

  void SetAll() noexcept;

  void Reset() noexcept;

 private:
  yaclib_std::atomic_uint8_t _state = 0;
};

}  // namespace yaclib::detail
