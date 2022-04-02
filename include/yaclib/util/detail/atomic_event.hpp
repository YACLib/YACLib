#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/atomic.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/ref.hpp>

namespace yaclib::detail {

class /*alignas(kCacheLineSize)*/ AtomicEvent : public IRef {
 public:
  using Token = Unit;

  Token Make() noexcept;

  void Wait(Token) noexcept;

  void Reset(Token) noexcept;

  void Set() noexcept;

 private:
  yaclib_std::atomic_uint8_t _state = 0;
};

}  // namespace yaclib::detail
