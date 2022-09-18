#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <utility>

namespace yaclib::detail {

struct Callback {
  IRef* caller;
  char unwrapping;
};

template <typename V, typename E>
class ResultCore : public BaseCore {
 public:
  ResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename... Args>
  explicit ResultCore(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>)
    : BaseCore{kResult}, _self{std::in_place_index_t<1>{}, std::forward<Args>(args)...} {
  }

  template <typename T>
  void Store(T&& object) noexcept(std::is_nothrow_constructible_v<Result<V, E>, T&&>) {
    _self = Result<V, E>{std::forward<T>(object)};
  }

  [[nodiscard]] Result<V, E>& Get() noexcept {
    return std::get<1>(_self);
  }
  [[nodiscard]] YACLIB_INLINE Callback& GetArgument() noexcept {
    return std::get<2>(_self);
  }

 public:
  std::variant<std::monostate, Result<V, E>, Callback> _self{std::monostate{}};
};

extern template class ResultCore<void, StopError>;

template <typename V, typename E>
using ResultCorePtr = IntrusivePtr<ResultCore<V, E>>;

template <typename V, typename E>
class DoneCore : ResultCore<V, E> {};

}  // namespace yaclib::detail
