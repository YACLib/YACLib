#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <utility>

namespace yaclib {

struct StopException final : std::exception {
  const char* what() const noexcept final {
    return "yaclib::StopException";
  }
};

template <typename T = void>
class Result {
  using V = std::conditional_t<std::is_void_v<T>, Unit, T>;

 public:
  Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<V>) {
    if (other) {
      new (&_value) V{std::move(other._value)};
      _error = {};
    } else {
      _error = other._error;
    }
  }

  Result(const Result& other) {
    if (other) {
      new (&_value) V{other._value};
      _error = {};
    } else {
      _error = other._error;
    }
  }

  Result& operator=(Result&& other) noexcept(std::is_nothrow_move_constructible_v<V> &&
                                             std::is_nothrow_move_assignable_v<V>) {
    if (this != &other) {
      if (*this && other) {
        _value = std::move(other._value);
      } else if (*this) {
        _error = other._error;
        _value.~V();
      } else if (other) {
        new (&_value) V{std::move(other._value)};
        _error = {};
      } else {
        _error = other._error;
      }
    }
    return *this;
  }

  Result& operator=(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<V> &&
                                                  std::is_nothrow_copy_assignable_v<V>) {
    if (this != &other) {
      if (*this && other) {
        _value = other._value;
      } else if (*this) {
        _error = other._error;
        _value.~V();
      } else if (other) {
        new (&_value) V{other._value};
        _error = {};
      } else {
        _error = other._error;
      }
    }
    return *this;
  }

  Result(std::exception_ptr error) noexcept : _error{std::move(error)} {
  }

  Result() : _value{} {
  }

  template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 1) ||
                                                          !std::is_same_v<std::decay_t<head_t<Args&&...>>, Result>>>
  Result(Args&&... args) : _value{std::forward<Args>(args)...} {
  }

  Result(StopTag) : _error{std::make_exception_ptr(StopException{})} {
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return !_error;
  }

  void Ok() & = delete;
  void Ok() const&& = delete;
  void Value() & = delete;
  void Value() const&& = delete;
  void Error() & = delete;
  void Error() const&& = delete;

  [[nodiscard]] V&& Ok() && {
    return Get(std::move(*this));
  }
  [[nodiscard]] const V& Ok() const& {
    return Get(*this);
  }

  [[nodiscard]] V&& Value() && noexcept {
    YACLIB_ASSERT(*this);
    return std::move(_value);
  }
  [[nodiscard]] const V& Value() const& noexcept {
    YACLIB_ASSERT(*this);
    return _value;
  }

  [[nodiscard]] const std::exception_ptr& Error() && noexcept {
    YACLIB_ASSERT(!*this);
    // Intentionally doesn't move _error here, because it's also control _value lifetime
    return _error;
  }
  [[nodiscard]] const std::exception_ptr& Error() const& noexcept {
    YACLIB_ASSERT(!*this);
    return _error;
  }

  ~Result() {
    if (*this) {
      _value.~V();
    }
  }

 private:
  template <typename R>
  static decltype(auto) Get(R&& r) {
    if (!r) {
      std::rethrow_exception(std::forward<R>(r).Error());
    }
    return std::forward<R>(r).Value();
  }

  std::exception_ptr _error;
  union {
    V _value;
  };
};

extern template class Result<>;

struct ResultTrait {
  template <typename V>
  using Result = Result<V>;

  template <typename V>
  using Error = std::exception_ptr;

  template <typename R>
  using Value = typename detail::InstantiationTypes<Result, R>::Value;

  template <typename V, typename... Args>
  YACLIB_INLINE static Result<V> MakeResult(Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
      return Result<V>{};
    } else {
      using Head = std::decay_t<head_t<Args&&...>>;
      if constexpr (std::is_same_v<Head, StopTag>) {
        return Result<V>{std::make_exception_ptr(StopException{})};
      } else if constexpr (std::is_same_v<Head, Unit>) {
        return Result<V>{};
      } else {
        return Result<V>{std::forward<Args>(args)...};
      }
    }
  }

  // template <typename V>
  YACLIB_INLINE static bool Ok(const auto& r) {
    return static_cast<bool>(r);
  }

  template <typename V>
  YACLIB_INLINE static decltype(auto) MoveValue(Result<V>&& r) {
    YACLIB_ASSERT(r);
    return std::move(r).Value();
  }

  template <typename V>
  YACLIB_INLINE static decltype(auto) MoveError(Result<V>&& r) {
    YACLIB_ASSERT(r);
    return std::move(r).Error();
  }
};

// struct to make it possible forward declared it
struct DefaultTrait : ResultTrait {};

}  // namespace yaclib
