#pragma once

#include <yaclib/lazy/schedule.hpp>
#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {

template <typename V, typename T>
class ReadyCore : public ResultCore<V, T> {
  using Result = typename T::template Result<V>;

 public:
  template <typename... Args>
  YACLIB_INLINE explicit ReadyCore(Unit, Args&&... args) {
    this->Store(std::forward<Args>(args)...);
  }

  void Call() noexcept final {
    Loop(this, this->template SetResult<false>());
  }

  void Drop() noexcept final {
    this->_result.~Result();
    this->Store(StopTag{});
    Call();
  }

  [[nodiscard]] InlineCore* Here(InlineCore& /*caller*/) noexcept final {
    return this->template SetResult<false>();
  }
#if YACLIB_SYMMETRIC_TRANSFER != 0
  [[nodiscard]] yaclib_std::coroutine_handle<> Next(InlineCore& /*caller*/) noexcept final {
    return this->template SetResult<true>();
  }
#endif
};

}  // namespace detail

/**
 * TODO(MBkkt) add description
 */
template <typename V = Unit, typename T = DefaultTrait, typename... Args>
/*Task*/ auto MakeTask(Args&&... args) {
  if constexpr (sizeof...(Args) == 0) {
    using Value = std::conditional_t<std::is_same_v<V, Unit>, void, V>;
    return Task{detail::ResultCorePtr<Value, T>{MakeUnique<detail::ReadyCore<Value, T>>(Unit{})}};
  } else if constexpr (std::is_same_v<V, Unit>) {
    using T0 = std::decay_t<head_t<Args&&...>>;
    using Value = std::conditional_t<std::is_same_v<T0, Unit>, void, T0>;
    return Task{
      detail::ResultCorePtr<Value, T>{MakeUnique<detail::ReadyCore<Value, T>>(Unit{}, std::forward<Args>(args)...)}};
  } else {
    return Task{detail::ResultCorePtr<V, T>{MakeUnique<detail::ReadyCore<V, T>>(Unit{}, std::forward<Args>(args)...)}};
  }
}

}  // namespace yaclib
