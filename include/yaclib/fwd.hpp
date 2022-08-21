#pragma once

namespace yaclib {

template <typename V, typename E>
class Result;

template <typename V, typename E>
class Task;

template <typename V, typename E>
class FutureBase;

template <typename V, typename E>
class Future;

template <typename V, typename E>
class FutureOn;

template <typename V, typename E>
class Promise;

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
