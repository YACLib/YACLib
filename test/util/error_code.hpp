#pragma once

#include <yaclib/util/result.hpp>

#include <system_error>
#include <variant>

namespace test {

struct LikeErrorCode : std::error_code {
  using std::error_code::error_code;

  LikeErrorCode(std::error_code error) noexcept : std::error_code{error} {
  }

  LikeErrorCode(yaclib::StopTag /*tag*/) noexcept
    : std::error_code{std::make_error_code(std::errc::operation_canceled)} {
  }

  LikeErrorCode(LikeErrorCode&&) noexcept = default;
  LikeErrorCode(const LikeErrorCode&) noexcept = default;
  LikeErrorCode& operator=(LikeErrorCode&&) noexcept = default;
  LikeErrorCode& operator=(const LikeErrorCode&) noexcept = default;

  const char* What() const noexcept {
    return this->category().name();
  }
};

/**
 * Minimal std::expected-like result container with LikeErrorCode errors.
 *
 * Together with \ref ErrorCodeTrait it proves that yaclib abstractions work with
 * arbitrary user result containers plugged in via result traits.
 */
template <typename ValueT>
class Expected final {
  using V = std::conditional_t<std::is_void_v<ValueT>, yaclib::Unit, ValueT>;
  using Variant = std::variant<V, LikeErrorCode>;

 public:
  Expected() noexcept : _result{std::in_place_index<1>, LikeErrorCode{yaclib::StopTag{}}} {
  }

  Expected(yaclib::StopTag tag) noexcept : _result{std::in_place_index<1>, LikeErrorCode{tag}} {
  }

  Expected(LikeErrorCode error) noexcept : _result{std::in_place_index<1>, error} {
  }

  template <typename... Args>
  explicit Expected(std::in_place_t, Args&&... args) : _result{std::in_place_index<0>, std::forward<Args>(args)...} {
  }

  template <typename... Args,
            typename = std::enable_if_t<(sizeof...(Args) > 1) ||
                                        !(std::is_same_v<std::decay_t<yaclib::head_t<Args&&...>>, Expected> ||
                                          std::is_same_v<std::decay_t<yaclib::head_t<Args&&...>>, LikeErrorCode> ||
                                          std::is_same_v<std::decay_t<yaclib::head_t<Args&&...>>, yaclib::StopTag> ||
                                          std::is_same_v<std::decay_t<yaclib::head_t<Args&&...>>, std::in_place_t>)>>
  Expected(Args&&... args) : _result{std::in_place_index<0>, std::forward<Args>(args)...} {
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return _result.index() == 0;
  }

  [[nodiscard]] V&& Ok() && {
    if (*this) {
      return std::move(*std::get_if<0>(&_result));
    }
    throw std::system_error{Error()};
  }
  [[nodiscard]] const V& Ok() const& {
    if (*this) {
      return *std::get_if<0>(&_result);
    }
    throw std::system_error{Error()};
  }

  [[nodiscard]] V&& Value() && noexcept {
    return std::move(*std::get_if<0>(&_result));
  }
  [[nodiscard]] const V& Value() const& noexcept {
    return *std::get_if<0>(&_result);
  }

  [[nodiscard]] LikeErrorCode&& Error() && noexcept {
    return std::move(*std::get_if<1>(&_result));
  }
  [[nodiscard]] const LikeErrorCode& Error() const& noexcept {
    return *std::get_if<1>(&_result);
  }

 private:
  Variant _result;
};

/**
 * Result trait that plugs \ref Expected into yaclib, \see yaclib::ResultTrait for the contract
 */
struct ErrorCodeTrait {
  template <typename V>
  using Result = Expected<V>;

  using Error = LikeErrorCode;

  template <typename R>
  using Value = typename yaclib::detail::InstantiationType<Expected, R>::Value;

  static LikeErrorCode FromException(std::exception_ptr e) noexcept {
    try {
      std::rethrow_exception(std::move(e));
    } catch (const yaclib::StopException&) {
      return LikeErrorCode{yaclib::StopTag{}};
    } catch (const std::system_error& error) {
      return LikeErrorCode{error.code()};
    } catch (...) {
      return LikeErrorCode{std::make_error_code(std::errc::io_error)};
    }
  }

  template <typename V, typename... Args>
  static Expected<V> MakeResult(Args&&... args) {
    static_assert(sizeof...(Args) > 0);
    using Head = std::decay_t<yaclib::head_t<Args&&...>>;
    if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, yaclib::Unit>) {
      return Expected<V>{std::in_place};
    } else if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, std::exception_ptr>) {
      return Expected<V>{FromException(std::forward<Args>(args)...)};
    } else if constexpr (std::is_same_v<Head, std::in_place_t> ||
                         (sizeof...(Args) == 1 &&
                          (std::is_same_v<Head, yaclib::StopTag> || std::is_same_v<Head, LikeErrorCode> ||
                           std::is_same_v<Head, Expected<V>>))) {
      return Expected<V>{std::forward<Args>(args)...};
    } else {
      return Expected<V>{std::in_place, std::forward<Args>(args)...};
    }
  }

  template <typename V>
  static bool Ok(const Expected<V>& r) noexcept {
    return static_cast<bool>(r);
  }

  template <typename R>
  static decltype(auto) GetValue(R&& r) noexcept {
    return std::forward<R>(r).Value();
  }

  template <typename R>
  static decltype(auto) GetError(R&& r) noexcept {
    return std::forward<R>(r).Error();
  }

  template <typename R>
  static decltype(auto) Get(R&& r) {
    return std::forward<R>(r).Ok();
  }

  static bool IsStop(const LikeErrorCode& error) noexcept {
    return error == LikeErrorCode{yaclib::StopTag{}};
  }
};

}  // namespace test
