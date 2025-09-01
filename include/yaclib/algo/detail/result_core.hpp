#pragma once

#include <yaclib/algo/detail/base_core.hpp>
#include <yaclib/util/cast.hpp>
#include <yaclib/util/result.hpp>

#include <utility>

namespace yaclib::detail {

struct Callback {
  IRef* caller = nullptr;
  char unwrapping = 0;
};

template <typename V, typename E>
class ResultCore : public BaseCore {
 public:
  using Value = V;
  using Error = E;

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

  [[nodiscard]] const Result<V, E>& GetConst() const noexcept {
    return _result;
  }

  template <bool Condition>
  decltype(auto) MoveOrConst() {
    if constexpr (Condition) {
      return std::move(Get());
    } else {
      return std::as_const(Get());
    }
  }

  union {
    Result<V, E> _result;
    Callback _self;
  };

 protected:
  template <bool SymmetricTransfer, bool Shared>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if constexpr (std::is_copy_constructible_v<Result<V, E>>) {
      // Copy values can come from both Unique and Shared cores
      const auto ref = caller.GetRef();
      if (ref >= 3) {
        // This is a Shared core and Shared futures exist and/or not
        // the last callback in the list, the value may not be moved
        ResultCore<V, E>::Store(DownCast<ResultCore<V, E>>(caller).Get());
        return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
      }
      // ref == 1: This is a Unique core, move the value and destroy the core
      // ref == 2: This is a Shared core, no more SharedFutures exist and the
      // last callback in the list, move the value but do not destroy the core
      ResultCore<V, E>::Store(std::move(DownCast<ResultCore<V, E>>(caller).Get()));
      if (ref == 1) {
        caller.DecRef();
      }
      return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
    } else if constexpr (std::is_move_constructible_v<Result<V, E>>) {
      // Move-only values are from Unique cores only
      ResultCore<V, E>::Store(std::move(DownCast<ResultCore<V, E>>(caller).Get()));
      caller.DecRef();
      return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
    } else {
      // Unreachable, cannot set callbacks on immovable cores
      YACLIB_PURE_VIRTUAL();
      return Noop<SymmetricTransfer>();
    }
  }
};

}  // namespace yaclib::detail
