#pragma once

namespace yaclib {

#define YACLIB_DEFINE_VOID_COMPARE(type)                                                                               \
  constexpr bool operator==(type, type) noexcept {                                                                     \
    return true;                                                                                                       \
  }                                                                                                                    \
  constexpr bool operator!=(type, type) noexcept {                                                                     \
    return false;                                                                                                      \
  }                                                                                                                    \
  constexpr bool operator<(type, type) noexcept {                                                                      \
    return false;                                                                                                      \
  }                                                                                                                    \
  constexpr bool operator<=(type, type) noexcept {                                                                     \
    return true;                                                                                                       \
  }                                                                                                                    \
  constexpr bool operator>=(type, type) noexcept {                                                                     \
    return true;                                                                                                       \
  }                                                                                                                    \
  constexpr bool operator>(type, type) noexcept {                                                                      \
    return false;                                                                                                      \
  }

#define YACLIB_DEFINE_VOID_TYPE(type)                                                                                  \
  struct type {};                                                                                                      \
  YACLIB_DEFINE_VOID_COMPARE(type)

YACLIB_DEFINE_VOID_TYPE(Unit);
YACLIB_DEFINE_VOID_TYPE(StopTag);

struct DefaultTrait;

template <typename V = void, typename T = DefaultTrait>
class [[nodiscard]] Task;

template <typename V, typename T>
class FutureBase;

template <typename V = void, typename T = DefaultTrait>
class [[nodiscard]] Future;

template <typename V = void, typename T = DefaultTrait>
class [[nodiscard]] FutureOn;

template <typename V = void, typename T = DefaultTrait>
class [[nodiscard]] Promise;

}  // namespace yaclib
