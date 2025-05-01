#pragma once

#include <yaclib/log.hpp>

#include <cstdint>

namespace yaclib::detail {

class GuardState {
  static constexpr auto kMask = ~std::uintptr_t{1};

 public:
  GuardState() noexcept = default;

  explicit GuardState(void* ptr, bool owns) noexcept
    : _state{reinterpret_cast<std::uintptr_t>(ptr) | static_cast<std::uintptr_t>(owns)} {
  }

  GuardState(GuardState&& other) noexcept : _state{other._state} {
    other._state &= kMask;
  }

  GuardState& operator=(GuardState&& other) noexcept {
    Swap(other);
    return *this;
  }

  void Swap(GuardState& other) noexcept {
    std::swap(_state, other._state);
  }

  void* Ptr() const {
    return reinterpret_cast<void*>(_state & kMask);
  }

  bool Owns() const noexcept {
    return (_state & 1U) != 0;
  }

  void* LockState() noexcept {
    YACLIB_DEBUG(Owns(), "Cannot lock locked guard");
    _state |= 1U;
    return Ptr();
  }

  void* UnlockState() noexcept {
    YACLIB_DEBUG(!Owns(), "Cannot unlock not locked guard");
    _state &= kMask;
    return Ptr();
  }

  void* ReleaseState() noexcept {
    void* was = Ptr();
    _state = 0;
    return was;
  }

 protected:
  std::uintptr_t _state = 0;
};

}  // namespace yaclib::detail
