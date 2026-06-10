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

template <typename V, typename T>
class ResultCore : public BaseCore {
 public:
  using Value = V;
  using Trait = T;
  using Result = typename T::template Result<V>;

  ResultCore() noexcept : BaseCore{kEmpty} {
  }

  template <typename... Args>
  explicit ResultCore(std::in_place_t, Args&&... args) : BaseCore{kResult} {
    if constexpr (sizeof...(Args) == 0) {
      Store(Unit{});
    } else {
      Store(std::forward<Args>(args)...);
    }
  }

  ~ResultCore() noexcept override {
    YACLIB_ASSERT(_callback.load(std::memory_order_relaxed) == kResult);
    _result.~Result();
  }

  template <typename... Args>
  void Store(Args&&... args) {
    ::new (&_result) Result{T::template MakeResult<V>(std::forward<Args>(args)...)};
  }

  [[nodiscard]] Result& Get() noexcept {
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

  virtual Result Retire() = 0;

  union {
    Result _result;
    Callback _self;
  };

 protected:
  template <bool SymmetricTransfer, bool Shared>
  [[nodiscard]] YACLIB_INLINE auto Impl(InlineCore& caller) noexcept {
    if constexpr (std::is_copy_constructible_v<wrap_void_t<V>>) {
      // Copy values can come from both Unique and Shared cores
      const auto ref = caller.GetRef();
      if (ref >= 3) {
        // This is a Shared core and Shared futures exist and/or not
        // the last callback in the list, the value may not be moved
        ResultCore<V, T>::Store(DownCast<ResultCore<V, T>>(caller).Get());
        return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
      }
      // ref == 1: This is a Unique core, move the value and destroy the core
      // ref == 2: This is a Shared core, no more SharedFutures exist and the
      // last callback in the list, move the value but do not destroy the core
      ResultCore<V, T>::Store(std::move(DownCast<ResultCore<V, T>>(caller).Get()));
      if (ref == 1) {
        caller.DecRef();
      }
      return BaseCore::SetResultImpl<SymmetricTransfer, Shared>();
    } else if constexpr (std::is_move_constructible_v<wrap_void_t<V>>) {
      // Move-only values are from Unique cores only
      ResultCore<V, T>::Store(std::move(DownCast<ResultCore<V, T>>(caller).Get()));
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
