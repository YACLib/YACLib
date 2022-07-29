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

/**
 * For internal instead of void usage
 */
struct Unit final {};

constexpr bool operator==(const Unit&, const Unit&) noexcept {
  return true;
}

}  // namespace yaclib
