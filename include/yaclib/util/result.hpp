#pragma once

#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <variant>

namespace yaclib {

/**
 * Result states \see Result
 * \enum Empty, Value, Error, Exception
 */
enum class ResultState : char {
  Value = 0,
  Exception = 1,
  Error = 2,
  Empty = 3,
};

struct StopTag {};

/**
 * Default error
 */
struct StopError {
  constexpr StopError(StopTag) noexcept {
  }
  constexpr StopError(StopError&&) noexcept = default;
  constexpr StopError(const StopError&) noexcept = default;
  constexpr StopError& operator=(StopError&&) noexcept = default;
  constexpr StopError& operator=(const StopError&) noexcept = default;

  constexpr bool operator==(const StopError& /*other*/) const noexcept {
    return true;
  }
};

/**
 * \class Exception for Error
 * \see Result
 */
template <typename Error>
class ResultError : public std::exception {
 public:
  ResultError(ResultError&&) noexcept(std::is_nothrow_move_constructible_v<Error>) = default;
  ResultError(const ResultError&) noexcept(std::is_nothrow_copy_constructible_v<Error>) = default;
  ResultError& operator=(ResultError&&) noexcept(std::is_nothrow_move_assignable_v<Error>) = default;
  ResultError& operator=(const ResultError&) noexcept(std::is_nothrow_copy_assignable_v<Error>) = default;

  explicit ResultError(Error&& error) noexcept(std::is_nothrow_copy_constructible_v<Error>) : _error{std::move(error)} {
  }

  [[nodiscard]] Error& Get() noexcept {
    return _error;
  }

 private:
  Error _error;
};

/**
 * \class Exception for Empty, invalid state
 * \see Result
 */
struct ResultEmpty : std::exception {};

/**
 * Encapsulated return value from caller
 *
 * \tparam V type of value that stored in Result
 * \tparam E type of error that stored in Result
 */
template <typename V, typename E = StopError>
class [[nodiscard]] Result {
  static_assert(Check<V>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<V, E>, "Result cannot be instantiated with same V and E, because it's ambiguous");
  static_assert(std::is_constructible_v<E, StopTag>, "Error should be constructable from StopError");

  using ValueT = std::conditional_t<std::is_void_v<V>, Unit, V>;
  using LetValue = std::conditional_t<std::is_void_v<V>, void, const ValueT&>;
  using MutValue = std::conditional_t<std::is_void_v<V>, void, ValueT&&>;
  using Variant = std::variant<ValueT, std::exception_ptr, E, std::monostate>;

 public:
  Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<Variant>) = default;
  Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<Variant>) = default;
  Result& operator=(Result&& other) noexcept(std::is_nothrow_move_assignable_v<Variant>) = default;
  Result& operator=(const Result& other) noexcept(std::is_nothrow_copy_assignable_v<Variant>) = default;

  template <typename... Args>
  Result(Args&&... args) noexcept(std::is_nothrow_constructible_v<Variant, std::in_place_type_t<ValueT>, Args...>)
      : _result{std::in_place_type_t<ValueT>{}, std::forward<Args>(args)...} {
  }

  Result(std::exception_ptr exception) noexcept
      : _result{std::in_place_type_t<std::exception_ptr>{}, std::move(exception)} {
  }

  Result(E error) noexcept : _result{std::in_place_type_t<E>{}, std::move(error)} {
  }

  Result(StopTag tag) noexcept : _result{std::in_place_type_t<E>{}, tag} {
  }

  Result() noexcept : _result{std::monostate{}} {
  }

  template <typename Arg>
  Result& operator=(Arg&& arg) noexcept(std::is_nothrow_assignable_v<Variant, Arg>) {
    _result = std::forward<Arg>(arg);
    return *this;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return State() == ResultState::Value;
  }

  /*[[nodiscard]]*/ MutValue Ok() && {
    switch (State()) {
      case ResultState::Value:
        if constexpr (std::is_void_v<V>) {
          return;
        } else {
          return std::move(*this).Value();
        }
      case ResultState::Exception:
        std::rethrow_exception(std::move(*this).Exception());
      case ResultState::Error:
        throw ResultError{std::move(*this).Error()};
      default:
        throw ResultEmpty{};
    }
  }

  /*[[nodiscard]]*/ LetValue Ok() const& {
    switch (State()) {
      case ResultState::Value:
        if constexpr (std::is_void_v<V>) {
          return;
        } else {
          return Value();
        }
      case ResultState::Exception:
        std::rethrow_exception(Exception());
      case ResultState::Error:
        throw ResultError{Error()};
      default:
        throw ResultEmpty{};
    }
  }

  [[nodiscard]] ResultState State() const noexcept {
    return ResultState{static_cast<char>(_result.index())};
  }

  [[nodiscard]] ValueT&& Value() && noexcept {
    return std::get<ValueT>(std::move(_result));
  }

  [[nodiscard]] const ValueT& Value() const& noexcept {
    return std::get<ValueT>(_result);
  }

  [[nodiscard]] std::exception_ptr&& Exception() && noexcept {
    return std::get<std::exception_ptr>(std::move(_result));
  }

  [[nodiscard]] const std::exception_ptr& Exception() const& noexcept {
    return std::get<std::exception_ptr>(_result);
  }

  [[nodiscard]] E&& Error() && noexcept {
    return std::get<E>(std::move(_result));
  }

  [[nodiscard]] const E& Error() const& noexcept {
    return std::get<E>(_result);
  }

  template <typename T>
  [[nodiscard]] T&& Extract() && noexcept {
    return std::get<T>(std::move(_result));
  }

 private:
  Variant _result;
};

}  // namespace yaclib
