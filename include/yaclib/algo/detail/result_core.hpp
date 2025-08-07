#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/result.hpp>

namespace yaclib::detail {

struct Callback {
  IRef* caller = nullptr;
  char unwrapping = 0;
};

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
    YACLIB_ASSERT(_callback.load(std::memory_order_relaxed) == kResult);
    _result.~Result<V, E>();
  }

  template <typename... Args>
  void Store(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>) {
    new (&_result) Result<V, E>{std::forward<Args>(args)...};
  }

  [[nodiscard]] Result<V, E>& Get() noexcept {
    return _result;
  }

  union {
    Result<V, E> _result;
    Callback _self;
  };
};

}  // namespace yaclib::detail
