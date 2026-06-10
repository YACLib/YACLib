#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/type_traits.hpp>

#include <exception>
#include <new>
#include <type_traits>
#include <utility>

namespace yaclib {

// [[msvc::no_unique_address]] on the all empty union below miscompiles with MSVC
// (runtime SEH in every Result<void> path, VS2022), so the empty value optimization
// is disabled there and sizeof(Result<void>) stays sizeof(exception_ptr) + 8
#if defined(_MSC_VER) && !defined(__clang__)
#  define YACLIB_RESULT_EMPTY_VALUE
#else
#  define YACLIB_RESULT_EMPTY_VALUE YACLIB_NO_UNIQUE_ADDRESS
#endif

/**
 * Exception that represents cancellation of an async operation, \see StopPtr
 */
struct StopException : std::exception {
  const char* what() const noexcept override {
    return "yaclib::StopException";
  }
};

/**
 * Cached std::exception_ptr with \ref StopException, copy is cheap and never allocates
 */
[[nodiscard]] const std::exception_ptr& StopPtr() noexcept;

/**
 * Check if error represents cancellation, \see StopException
 */
[[nodiscard]] bool IsStop(const std::exception_ptr& error) noexcept;

/**
 * Encapsulated return value from caller
 *
 * Either a value of type V or a std::exception_ptr.
 * Default constructed Result contains the stop error, \see StopPtr
 *
 * \tparam ValueT type of value that stored in Result
 */
template <typename ValueT>
class [[nodiscard]] Result final {
  static_assert(Check<ValueT>(), "V should be valid");

  using V = std::conditional_t<std::is_void_v<ValueT>, Unit, ValueT>;

  union State {
    YACLIB_RESULT_EMPTY_VALUE Unit stub;
    YACLIB_RESULT_EMPTY_VALUE V value;

    State() noexcept : stub{} {
    }
    ~State() noexcept {
    }
  };

 public:
  Result() noexcept : _error{StopPtr()} {
  }

  Result(StopTag) noexcept : _error{StopPtr()} {
  }

  Result(std::exception_ptr error) noexcept : _error{std::move(error)} {
    // null exception_ptr would be indistinguishable from a value, treat it as stop
    YACLIB_ASSERT(_error != nullptr);
    if (_error == nullptr) {
      _error = StopPtr();
    }
  }

  template <typename... Args>
  explicit Result(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<V, Args&&...>) {
    ::new (&_value.value) V(std::forward<Args>(args)...);
  }

  template <typename... Args,
            typename = std::enable_if_t<(sizeof...(Args) > 1) ||
                                        !(std::is_same_v<std::decay_t<head_t<Args&&...>>, Result> ||
                                          std::is_same_v<std::decay_t<head_t<Args&&...>>, std::exception_ptr> ||
                                          std::is_same_v<std::decay_t<head_t<Args&&...>>, StopTag> ||
                                          std::is_same_v<std::decay_t<head_t<Args&&...>>, std::in_place_t>)>>
  Result(Args&&... args) noexcept(std::is_nothrow_constructible_v<V, Args&&...>)
    : Result{std::in_place, std::forward<Args>(args)...} {
  }

  // _error is copied, not moved: it's also the discriminant that controls the value lifetime
  Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<V>) : _error{other._error} {
    if (!_error) {
      ::new (&_value.value) V(other._value.value);
    }
  }

  Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<V>) : _error{other._error} {
    if (!_error) {
      ::new (&_value.value) V(std::move(other._value.value));
    }
  }

  Result& operator=(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<V> &&
                                                  std::is_nothrow_copy_assignable_v<V>) {
    if (this != &other) {
      if (!other._error) {
        if (!_error) {
          _value.value = other._value.value;
        } else {
          ::new (&_value.value) V(other._value.value);
          _error = nullptr;
        }
      } else {
        if (!_error) {
          _value.value.~V();
        }
        _error = other._error;
      }
    }
    return *this;
  }

  Result& operator=(Result&& other) noexcept(std::is_nothrow_move_constructible_v<V> &&
                                             std::is_nothrow_move_assignable_v<V>) {
    if (this != &other) {
      if (!other._error) {
        if (!_error) {
          _value.value = std::move(other._value.value);
        } else {
          ::new (&_value.value) V(std::move(other._value.value));
          _error = nullptr;
        }
      } else {
        if (!_error) {
          _value.value.~V();
        }
        _error = other._error;
      }
    }
    return *this;
  }

  template <typename Arg, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Arg>, Result>>>
  Result& operator=(Arg&& arg) {
    *this = Result{std::forward<Arg>(arg)};
    return *this;
  }

  ~Result() noexcept {
    if (!_error) {
      _value.value.~V();
    }
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
    if (_error) {
      std::rethrow_exception(_error);
    }
    return std::move(_value.value);
  }
  [[nodiscard]] const V& Ok() const& {
    if (_error) {
      std::rethrow_exception(_error);
    }
    return _value.value;
  }

  [[nodiscard]] V&& Value() && noexcept {
    YACLIB_ASSERT(!_error);
    return std::move(_value.value);
  }
  [[nodiscard]] const V& Value() const& noexcept {
    YACLIB_ASSERT(!_error);
    return _value.value;
  }

  [[nodiscard]] const std::exception_ptr& Error() && noexcept {
    YACLIB_ASSERT(_error);
    return _error;
  }
  [[nodiscard]] const std::exception_ptr& Error() const& noexcept {
    YACLIB_ASSERT(_error);
    return _error;
  }

 private:
  std::exception_ptr _error;
  YACLIB_RESULT_EMPTY_VALUE State _value;
};

extern template class Result<>;

/**
 * Default result trait, describes how async abstractions create and inspect results
 *
 * A custom trait should provide the same interface, its container should be copyable iff V is copyable.
 * MakeResult<V>(StopTag) and MakeResult<V>(std::exception_ptr) must not throw: they are invoked
 * inside noexcept cancellation and teardown paths, a throw there will std::terminate
 */
struct ResultTrait {
  template <typename V>
  using Result = yaclib::Result<V>;

  using Error = std::exception_ptr;

  template <typename R>
  using Value = typename detail::InstantiationType<yaclib::Result, R>::Value;

  template <typename V, typename... Args>
  YACLIB_INLINE static yaclib::Result<V> MakeResult(Args&&... args) {
    static_assert(sizeof...(Args) > 0, "Use MakeResult<V>(Unit{}) to construct default value");
    using Head = std::decay_t<head_t<Args&&...>>;
    if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, Unit>) {
      return yaclib::Result<V>{std::in_place};
    } else if constexpr (std::is_same_v<Head, std::in_place_t> ||
                         (sizeof...(Args) == 1 &&
                          (std::is_same_v<Head, StopTag> || std::is_same_v<Head, std::exception_ptr> ||
                           std::is_same_v<Head, yaclib::Result<V>>))) {
      return yaclib::Result<V>{std::forward<Args>(args)...};
    } else {
      return yaclib::Result<V>{std::in_place, std::forward<Args>(args)...};
    }
  }

  template <typename V>
  YACLIB_INLINE static bool Ok(const yaclib::Result<V>& r) noexcept {
    return static_cast<bool>(r);
  }

  template <typename R>
  YACLIB_INLINE static decltype(auto) GetValue(R&& r) noexcept {
    return std::forward<R>(r).Value();
  }

  template <typename R>
  YACLIB_INLINE static decltype(auto) GetError(R&& r) noexcept {
    return std::forward<R>(r).Error();
  }

  template <typename R>
  YACLIB_INLINE static decltype(auto) Get(R&& r) {
    return std::forward<R>(r).Ok();
  }

  YACLIB_INLINE static bool IsStop(const std::exception_ptr& error) noexcept {
    return yaclib::IsStop(error);
  }
};

/**
 * Trait used by default for all async abstractions, a separate struct so it can be forward declared
 */
struct DefaultTrait : ResultTrait {};

}  // namespace yaclib
