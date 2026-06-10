#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <type_traits>
#include <utility>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

namespace yaclib_ext {

/**
 * Result trait that plugs absl::StatusOr<V> into yaclib, \see yaclib::ResultTrait for the contract
 *
 * Result<void> is absl::Status. Cancellation is absl::StatusCode::kCancelled.
 * Exceptions thrown by user callbacks are converted to absl::StatusCode::kUnknown.
 */
struct StatusOrTrait {
  template <typename V>
  using Result = std::conditional_t<std::is_void_v<V>, absl::Status, absl::StatusOr<V>>;

  using Error = absl::Status;

  template <typename R>
  using Value = std::conditional_t<std::is_same_v<R, absl::Status>, void,
                                   typename yaclib::detail::InstantiationType<absl::StatusOr, R>::Value>;

  static absl::Status FromException(std::exception_ptr e) noexcept {
    try {
      std::rethrow_exception(std::move(e));
    } catch (const yaclib::StopException&) {
      return absl::CancelledError("yaclib::StopException");
    } catch (const std::exception& exception) {
      return absl::UnknownError(exception.what());
    } catch (...) {
      return absl::UnknownError("unknown exception");
    }
  }

  template <typename V, typename... Args>
  static Result<V> MakeResult(Args&&... args) {
    static_assert(sizeof...(Args) > 0);
    using Head = std::decay_t<yaclib::head_t<Args&&...>>;
    if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, yaclib::Unit>) {
      if constexpr (std::is_void_v<V>) {
        return absl::OkStatus();
      } else {
        return Result<V>{std::in_place};
      }
    } else if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, yaclib::StopTag>) {
      return Result<V>{absl::CancelledError("yaclib::StopTag")};
    } else if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, std::exception_ptr>) {
      return Result<V>{FromException(std::forward<Args>(args)...)};
    } else if constexpr (std::is_same_v<Head, std::in_place_t> ||
                         (sizeof...(Args) == 1 &&
                          (std::is_same_v<Head, absl::Status> || std::is_same_v<Head, Result<V>>))) {
      if constexpr (std::is_void_v<V> && std::is_same_v<Head, std::in_place_t>) {
        return absl::OkStatus();
      } else {
        return Result<V>{std::forward<Args>(args)...};
      }
    } else {
      return Result<V>{std::in_place, std::forward<Args>(args)...};
    }
  }

  // Two deducible overloads: Result<V> is a conditional_t alias, so V would be non-deduced
  static bool Ok(const absl::Status& r) noexcept {
    return r.ok();
  }
  template <typename U>
  static bool Ok(const absl::StatusOr<U>& r) noexcept {
    return r.ok();
  }

  template <typename R>
  static decltype(auto) GetValue(R&& r) noexcept {
    if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<R>>, absl::Status>) {
      return yaclib::Unit{};
    } else {
      return *std::forward<R>(r);
    }
  }

  template <typename R>
  static decltype(auto) GetError(R&& r) noexcept {
    if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<R>>, absl::Status>) {
      return std::forward<R>(r);
    } else {
      return std::forward<R>(r).status();
    }
  }

  template <typename R>
  static decltype(auto) Get(R&& r) {
    if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<R>>, absl::Status>) {
      if (!r.ok()) {
        throw absl::BadStatusOrAccess{std::forward<R>(r)};
      }
    } else {
      return std::forward<R>(r).value();
    }
  }

  static bool IsStop(const absl::Status& error) noexcept {
    return error.code() == absl::StatusCode::kCancelled;
  }
};

}  // namespace yaclib_ext
