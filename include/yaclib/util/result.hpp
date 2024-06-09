#pragma once

#include <yaclib/fwd.hpp>
#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <utility>
#include <variant>

namespace yaclib {

/**
 * Result states \see Result
 * \enum Value, Exception, Error, Empty
 */
enum class [[nodiscard]] ResultState : unsigned char {
  Value = 0,
  Exception = 1,
  Error = 2,
  Empty = 3,
};

/**
 * Default error
 */
struct [[nodiscard]] StopError final {
  constexpr StopError(StopTag) noexcept {
  }
  constexpr StopError(StopError&&) noexcept = default;
  constexpr StopError(const StopError&) noexcept = default;
  constexpr StopError& operator=(StopError&&) noexcept = default;
  constexpr StopError& operator=(const StopError&) noexcept = default;

  static const char* What() noexcept {
    return "yaclib::StopError";
  }
};

YACLIB_DEFINE_VOID_COMPARE(StopError)

/**
 * \class Exception for Error
 * \see Result
 */
template <typename Error>
class [[nodiscard]] ResultError final : public std::exception {
 public:
  ResultError(ResultError&&) noexcept(std::is_nothrow_move_constructible_v<Error>) = default;
  ResultError(const ResultError&) noexcept(std::is_nothrow_copy_constructible_v<Error>) = default;
  ResultError& operator=(ResultError&&) noexcept(std::is_nothrow_move_assignable_v<Error>) = default;
  ResultError& operator=(const ResultError&) noexcept(std::is_nothrow_copy_assignable_v<Error>) = default;

  explicit ResultError(Error&& error) noexcept(std::is_nothrow_move_constructible_v<Error>) : _error{std::move(error)} {
  }
  explicit ResultError(const Error& error) noexcept(std::is_nothrow_copy_constructible_v<Error>) : _error{error} {
  }

  [[nodiscard]] Error& Get() & noexcept {
    return _error;
  }
  [[nodiscard]] const Error& Get() const& noexcept {
    return _error;
  }

  const char* what() const noexcept final {
    return _error.What();
  }

 private:
  Error _error;
};

/**
 * \class Exception for Empty, invalid state
 * \see Result
 */
struct ResultEmpty final : std::exception {
  const char* what() const noexcept final {
    return "yaclib::ResultEmpty";
  }
};

/**
 * Encapsulated return value from caller
 *
 * \tparam ValueT type of value that stored in Result
 * \tparam E type of error that stored in Result
 */
template <typename ValueT, typename E>
class Result final {
  static_assert(Check<ValueT>(), "V should be valid");
  static_assert(Check<E>(), "E should be valid");
  static_assert(!std::is_same_v<ValueT, E>, "Result cannot be instantiated with same V and E, because it's ambiguous");
  static_assert(std::is_constructible_v<E, StopTag>, "Error should be constructable from StopTag");
  using V = std::conditional_t<std::is_void_v<ValueT>, Unit, ValueT>;
  using Variant = std::variant<V, std::exception_ptr, E, std::monostate>;

 public:
  Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<Variant>) = default;
  Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<Variant>) = default;
  Result& operator=(Result&& other) noexcept(std::is_nothrow_move_assignable_v<Variant>) = default;
  Result& operator=(const Result& other) noexcept(std::is_nothrow_copy_assignable_v<Variant>) = default;

  template <typename... Args,
            typename =
              std::enable_if_t<(sizeof...(Args) > 1 || !std::is_same_v<std::decay_t<head_t<Args&&...>>, Result>), void>>
  Result(Args&&... args) noexcept(std::is_nothrow_constructible_v<Variant, std::in_place_type_t<V>, Args&&...>)
    : Result{std::in_place, std::forward<Args>(args)...} {
  }

  template <typename... Args>
  Result(std::in_place_t,
         Args&&... args) noexcept(std::is_nothrow_constructible_v<Variant, std::in_place_type_t<V>, Args&&...>)
    : _result{std::in_place_type<V>, std::forward<Args>(args)...} {
  }

  Result(std::exception_ptr exception) noexcept
    : _result{std::in_place_type<std::exception_ptr>, std::move(exception)} {
  }

  Result(E error) noexcept : _result{std::in_place_type<E>, std::move(error)} {
  }

  Result(StopTag tag) noexcept : _result{std::in_place_type<E>, tag} {
  }

  Result() noexcept : _result{std::monostate{}} {
  }

  template <typename Arg, typename = std::enable_if_t<!is_result_v<std::decay_t<Arg>>, void>>
  Result& operator=(Arg&& arg) noexcept(std::is_nothrow_assignable_v<Variant, Arg>) {
    _result = std::forward<Arg>(arg);
    return *this;
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return State() == ResultState::Value;
  }

  void Ok() & = delete;
  void Ok() const&& = delete;
  void Value() & = delete;
  void Value() const&& = delete;
  void Exception() & = delete;
  void Exception() const&& = delete;
  void Error() & = delete;
  void Error() const&& = delete;

  [[nodiscard]] V&& Ok() && {
    return Get(std::move(*this));
  }
  [[nodiscard]] const V& Ok() const& {
    return Get(*this);
  }

  [[nodiscard]] ResultState State() const noexcept {
    return ResultState{static_cast<unsigned char>(_result.index())};
  }

  [[nodiscard]] V&& Value() && noexcept {
    return std::get<V>(std::move(_result));
  }
  [[nodiscard]] const V& Value() const& noexcept {
    return std::get<V>(_result);
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

  [[nodiscard]] Variant& Internal() {
    return _result;
  }

 private:
  template <typename R>
  static decltype(auto) Get(R&& r) {
    switch (r.State()) {
      case ResultState::Value:
        return std::forward<R>(r).Value();
      case ResultState::Exception:
        std::rethrow_exception(std::forward<R>(r).Exception());
      case ResultState::Error:
        throw ResultError{std::forward<R>(r).Error()};
      default:
        throw ResultEmpty{};
    }
  }

  Variant _result;
};

extern template class Result<>;

}  // namespace yaclib
