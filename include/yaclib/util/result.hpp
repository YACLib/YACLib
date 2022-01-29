#pragma once

#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <system_error>
#include <variant>

namespace yaclib::util {

template <typename T>
class Result;

namespace detail {

template <typename U>
struct ResultValue {
  using type = U;
};

template <typename U>
struct ResultValue<Result<U>> {
  using type = U;
};

template <typename T>
using ResultValueT = typename ResultValue<T>::type;

}  // namespace detail

struct Unit {};

/**
 * Result states \see Result
 * \enum Empty, Value, Error, Exception
 */
enum class ResultState : uint8_t {
  Value = 0,
  Exception = 1,
  Error = 2,
  Empty = 3,
};

/**
 * \class Exception for std::error_code
 * \see Result
 */
class ResultError : public std::exception {
 public:
  explicit ResultError(std::error_code error) noexcept : _error{error} {
  }
  [[nodiscard]] std::error_code Get() const noexcept {
    return _error;
  }

 private:
  std::error_code _error;
};

struct ResultEmpty : std::exception {};

/**
 * Encapsulated return value from caller
 *
 * \tparam T type to store in Result
 */
template <typename T>
class Result {
  static_assert(!std::is_reference_v<T>,
                "Result cannot be instantiated with reference, "
                "you can use std::reference_wrapper or pointer");
  static_assert(!std::is_volatile_v<T> && !std::is_const_v<T>,
                "Result cannot be instantiated with cv qualifiers, because it's unnecessary");
  static_assert(!util::IsResultV<T>, "Result cannot be instantiated with Result");
  static_assert(!util::IsFutureV<T>, "Result cannot be instantiated with Future");
  static_assert(!std::is_same_v<T, std::error_code>, "Result cannot be instantiated with std::error_code");
  static_assert(!std::is_same_v<T, std::exception_ptr>, "Result cannot be instantiated with std::exception_ptr");

  using ValueT = std::conditional_t<std::is_void_v<T>, Unit, T>;
  using VariantT = std::variant<ValueT, std::exception_ptr, std::error_code, std::monostate>;

 public:
  Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<VariantT>) = default;
  Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<VariantT>) = default;
  Result& operator=(Result&& other) noexcept(std::is_nothrow_move_assignable_v<VariantT>) = default;
  Result& operator=(const Result& other) noexcept(std::is_nothrow_copy_assignable_v<VariantT>) = default;

  constexpr Result() noexcept : _result{std::monostate{}} {
  }

  template <typename U>
  constexpr explicit Result(U&& value) noexcept(std::is_nothrow_constructible_v<VariantT, U>)
      : _result{std::forward<U>(value)} {
  }

  template <typename U>
  Result& operator=(U&& value) noexcept(std::is_nothrow_assignable_v<VariantT, U>) {
    _result = std::forward<U>(value);
    return *this;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return State() == ResultState::Value;
  }

  /*[[nodiscard]]*/ T Ok() && {
    switch (State()) {
      case ResultState::Value:
        return std::move(*this).Value();
      case ResultState::Exception:
        std::rethrow_exception(std::move(*this).Exception());
      case ResultState::Error:
        throw ResultError{std::move(*this).Error()};
      default:
        throw ResultEmpty{};
    }
  }

  [[nodiscard]] ResultState State() const noexcept {
    return ResultState{static_cast<uint8_t>(_result.index())};
  }

  [[nodiscard]] T Value() && noexcept {
    if constexpr (!std::is_void_v<T>) {
      return std::get<T>(std::move(_result));
    }
  }
  [[nodiscard]] std::error_code Error() && noexcept {
    return std::get<std::error_code>(std::move(_result));
  }
  [[nodiscard]] std::exception_ptr Exception() && noexcept {
    return std::get<std::exception_ptr>(std::move(_result));
  }

 private:
  VariantT _result;
};

}  // namespace yaclib::util
