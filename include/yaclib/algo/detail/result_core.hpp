#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <utility>

namespace yaclib::detail {

struct Callback {
  IRef* caller = nullptr;
  char unwrapping = 0;
};

template <typename V, typename E>
class ResultCore : public BaseCore {
  template <bool SymmetricTransfer>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& /*caller*/) noexcept {
    YACLIB_PURE_VIRTUAL();
    return Noop<SymmetricTransfer>();
  }

 public:
  [[nodiscard]] InlineCore* Here(InlineCore& caller) noexcept override {
    return Impl<false>(caller);
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& caller) noexcept override {
    return Impl<true>(caller);
  }
#endif

  ResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename... Args>
  explicit ResultCore(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>)
    : BaseCore{kResult}, _result{std::forward<Args>(args)...} {
  }

  ~ResultCore() noexcept override {
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

extern template class ResultCore<void, StopError>;

template <typename V, typename E>
using ResultCorePtr = IntrusivePtr<ResultCore<V, E>>;

}  // namespace yaclib::detail
