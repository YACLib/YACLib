#pragma once

#include <yaclib/lazy/schedule.hpp>
#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {

template <typename V, typename E>
class ReadyCore : public ResultCore<V, E> {
 public:
  template <typename... Args>
  ReadyCore(Args&&... args) noexcept(std::is_nothrow_constructible_v<Result<V, E>, Args&&...>) {
    this->Store(std::forward<Args>(args)...);
  }

  void Call() noexcept final {
    this->template SetResult<false>();
  }

  void Drop() noexcept final {
    this->_result.~Result<V, E>();
    this->Store(StopTag{});
    Call();
  }
};

}  // namespace detail

/**
 * TODO(MBkkt) add description
 */
template <typename V = Unit, typename E = StopError, typename... Args>
/*Task*/ auto MakeTask(Args&&... args) {
  if constexpr (sizeof...(Args) == 0) {
    using T = std::conditional_t<std::is_same_v<V, Unit>, void, V>;
    return Task{detail::ResultCorePtr<T, E>{MakeUnique<detail::ReadyCore<T, E>>(std::in_place)}};
  } else if constexpr (std::is_same_v<V, Unit>) {
    using T0 = std::decay_t<head_t<Args&&...>>;
    using T = std::conditional_t<std::is_same_v<T0, Unit>, void, T0>;
    return Task{
      detail::ResultCorePtr<T, E>{MakeUnique<detail::ReadyCore<T, E>>(std::in_place, std::forward<Args>(args)...)}};
  } else {
    return Task{detail::ResultCorePtr<V, E>{MakeUnique<detail::ReadyCore<V, E>>(std::forward<Args>(args)...)}};
  }
}

}  // namespace yaclib
