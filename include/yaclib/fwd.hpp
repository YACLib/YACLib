#pragma once

namespace yaclib {

struct [[nodiscard]] StopError;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Result;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Task;

template <typename V, typename E>
class [[nodiscard]] FutureBase;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Future;

template <typename V = void, typename E = StopError>
class [[nodiscard]] FutureOn;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Promise;

#define YACLIB_DEFINE_VOID_TYPE(type)                                                                                  \
  struct type {};                                                                                                      \
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

YACLIB_DEFINE_VOID_TYPE(Unit);
YACLIB_DEFINE_VOID_TYPE(StopTag);

}  // namespace yaclib
