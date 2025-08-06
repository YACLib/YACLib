#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib::detail {

struct Callback {
  IRef* caller = nullptr;
  char unwrapping = 0;
};

template <typename V, typename E>
struct ResultCore : BaseCore {
  union {
    Result<V, E> _result;
    Callback _self;
  };

  ResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename... Args>
  explicit ResultCore(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>)
    : BaseCore{kResult}, _result{std::forward<Args>(args)...} {
  }

  template <typename... Args>
  void Store(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>) {
    new (&_result) Result<V, E>{std::forward<Args>(args)...};
  }

  [[nodiscard]] Result<V, E>& Get() noexcept {
    return _result;
  }

  [[nodiscard]] const Result<V, E>& Get() const noexcept {
    return _result;
  }

  ~ResultCore() noexcept override {
    YACLIB_ASSERT(reinterpret_cast<std::uintptr_t>(_callback.load(std::memory_order_relaxed)) == kResult);
    _result.~Result<V, E>();
  }
};

}  // namespace yaclib::detail
