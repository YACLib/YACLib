#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <expected>
#include <type_traits>
#include <utility>

namespace yaclib_ext {

/**
 * Result trait that plugs std::expected<V, E> into yaclib, \see yaclib::ResultTrait for the contract
 *
 * Requirements for E:
 * - E(yaclib::StopTag) constructs the cancellation error
 * - E(std::exception_ptr) converts an exception thrown by user callback
 * - operator== to recognize cancellation, \see IsStop
 */
template <typename E>
struct ExpectedTrait {
  static_assert(std::is_constructible_v<E, yaclib::StopTag>, "E should be constructible from yaclib::StopTag");
  static_assert(std::is_constructible_v<E, std::exception_ptr>, "E should be constructible from std::exception_ptr");

  template <typename V>
  using Result = std::expected<V, E>;

  using Error = E;

  template <typename R>
  using Value = typename yaclib::detail::InstantiationTypes<std::expected, R>::Value;

  template <typename V, typename... Args>
  static Result<V> MakeResult(Args&&... args) {
    static_assert(sizeof...(Args) > 0);
    using Head = std::decay_t<yaclib::head_t<Args&&...>>;
    if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, yaclib::Unit>) {
      return Result<V>{std::in_place};
    } else if constexpr (sizeof...(Args) == 1 &&
                         (std::is_same_v<Head, yaclib::StopTag> || std::is_same_v<Head, std::exception_ptr> ||
                          std::is_same_v<Head, E>)) {
      return Result<V>{std::unexpect, E{std::forward<Args>(args)...}};
    } else if constexpr (std::is_same_v<Head, std::in_place_t> ||
                         (sizeof...(Args) == 1 && std::is_same_v<Head, Result<V>>)) {
      return Result<V>{std::forward<Args>(args)...};
    } else {
      return Result<V>{std::in_place, std::forward<Args>(args)...};
    }
  }

  template <typename V>
  static bool Ok(const Result<V>& r) noexcept {
    return r.has_value();
  }

  template <typename R>
  static decltype(auto) GetValue(R&& r) noexcept {
    if constexpr (std::is_void_v<typename std::remove_reference_t<R>::value_type>) {
      return yaclib::Unit{};
    } else {
      return *std::forward<R>(r);
    }
  }

  template <typename R>
  static decltype(auto) GetError(R&& r) noexcept {
    return std::forward<R>(r).error();
  }

  template <typename R>
  static decltype(auto) Get(R&& r) {
    return std::forward<R>(r).value();
  }

  static bool IsStop(const E& error) noexcept {
    return error == E{yaclib::StopTag{}};
  }
};

}  // namespace yaclib_ext
