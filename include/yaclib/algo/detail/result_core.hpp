#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <utility>

namespace yaclib::detail {

template <typename V, typename E>
class ResultCore : public BaseCore {
 public:
  ResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename... Args>
  explicit ResultCore(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>)
    : BaseCore{kResult}, _result{std::forward<Args>(args)...} {
  }

  ~ResultCore() noexcept override {
    _result.~Result<V, E>();
  }

  template <typename T>
  void Store(T&& object) noexcept(std::is_nothrow_constructible_v<Result<V, E>, T&&>) {
    new (&_result) Result<V, E>{std::forward<T>(object)};
  }

  [[nodiscard]] Result<V, E>& Get() noexcept {
    return _result;
  }

 protected:
  union {
    Result<V, E> _result;
    char _unwrapping;
  };
};

template <>
class ResultCore<void, void> : public BaseCore {
 public:
  ResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename T>
  void Store(T&&) noexcept {
  }
};

extern template class ResultCore<void, StopError>;

template <typename V, typename E>
using ResultCorePtr = IntrusivePtr<ResultCore<V, E>>;

}  // namespace yaclib::detail
