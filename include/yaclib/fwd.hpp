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

struct [[nodiscard]] StopError;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Result;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Task;

template <typename V, typename E>
class FutureBase;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Future;

template <typename V = void, typename E = StopError>
class [[nodiscard]] FutureOn;

template <typename V = void, typename E = StopError>
class [[nodiscard]] Promise;

template <typename V = void, typename E = StopError>
class [[nodiscard]] SharedFuture;

template <typename V = void, typename E = StopError>
class [[nodiscard]] SharedPromise;

namespace detail {
class InlineCore;

template <typename V, typename E>
class SharedCore;

template <typename V, typename E>
class ResultCore;

template <typename V, typename E, bool Shared>
decltype(auto) ResultFromCore(InlineCore& core);
}  // namespace detail

}  // namespace yaclib
