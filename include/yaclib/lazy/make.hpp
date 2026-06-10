#pragma once

#include <yaclib/lazy/schedule.hpp>
#include <yaclib/lazy/task.hpp>

namespace yaclib {
namespace detail {

template <typename V, typename T>
class ReadyCore : public UniqueCore<V, T> {
 public:
  using Result = typename UniqueCore<V, T>::Result;

  template <typename... Args>
  explicit ReadyCore(std::in_place_t, Args&&... args) {
    // Body level try is required: in a constructor function try block the handler runs
    // after the base is already destroyed and always rethrows at the end
    try {
      if constexpr (sizeof...(Args) == 0) {
        this->Store(Unit{});
      } else {
        this->Store(std::forward<Args>(args)...);
      }
    } catch (...) {
      this->Store(std::current_exception());
    }
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
    return Task{detail::UniqueCorePtr<Value, T>{MakeUnique<detail::ReadyCore<Value, T>>(std::in_place)}};
  } else if constexpr (std::is_same_v<V, Unit>) {
    using Head = std::decay_t<head_t<Args&&...>>;
    using Value = std::conditional_t<std::is_same_v<Head, Unit>, void, Head>;
    return Task{detail::UniqueCorePtr<Value, T>{
      MakeUnique<detail::ReadyCore<Value, T>>(std::in_place, std::forward<Args>(args)...)}};
  } else {
    return Task{
      detail::UniqueCorePtr<V, T>{MakeUnique<detail::ReadyCore<V, T>>(std::in_place, std::forward<Args>(args)...)}};
  }
}

}  // namespace yaclib
