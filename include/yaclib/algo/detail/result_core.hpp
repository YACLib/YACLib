#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/cast.hpp>
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

 protected:
  template <bool SymmetricTransfer, bool Shared>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if constexpr (std::is_move_constructible_v<Result<V, E>>) {
      auto refcount = caller.GetRef();
      if constexpr (std::is_copy_constructible_v<Result<V, E>>) {
        if (refcount >= 3) {
          // SharedCore, SharedFutures exist and/or not the last callback in the list
          // the value may not be moved
          ResultCore<V, E>::Store(DownCast<ResultCore<V, E>>(caller).Get());
          return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
        }
      }
      // refcount 1 : UniqueCore, move the value and destroy the core
      // refcount 2 : SharedCore, no more SharedFutures exist and the last callback in the list,
      // move the value but let the core destroy itself
      YACLIB_ASSERT(refcount == 1 || refcount == 2);
      ResultCore<V, E>::Store(std::move(DownCast<ResultCore<V, E>>(caller).Get()));
      if (refcount == 1) {
        caller.DecRef();
      }
      return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
    } else {
      YACLIB_PURE_VIRTUAL();
      return Noop<SymmetricTransfer>();
    }
  }
};

}  // namespace yaclib::detail
