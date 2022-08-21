#pragma once

#include <yaclib/lazy/schedule.hpp>
#include <yaclib/lazy/task.hpp>

namespace yaclib {

/**
 * TODO(MBkkt) add description
 */
template <typename V = Unit, typename E = StopError, typename... Args>
/*Task*/ auto MakeTask(Args&&... args) {
  // TODO(MBkkt) optimize sizeof: now we store Result twice
  if constexpr (sizeof...(Args) == 0) {
    return Schedule<E>(MakeInline(), [] {
    });
  } else if constexpr (sizeof...(Args) == 1) {
    using T = std::conditional_t<std::is_same_v<V, Unit>, head_t<Args...>, V>;
    static_assert(!std::is_same_v<T, Unit>);
    return Schedule<E>(MakeInline(), [result = Result<T, E>{std::forward<Args>(args)...}]() mutable {
      return std::move(result);
    });
  } else {
    static_assert(!std::is_same_v<V, Unit>);
    return Schedule<E>(MakeInline(), [result = Result<V, E>{std::forward<Args>(args)...}]() mutable {
      return std::move(result);
    });
  }
}

}  // namespace yaclib
