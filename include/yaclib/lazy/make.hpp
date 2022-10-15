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
    return Schedule<E>([] {
    });
  } else {
    using T = std::conditional_t<sizeof...(Args) == 1 && std::is_same_v<V, Unit>, std::decay_t<head_t<Args&&...>>, V>;
    static_assert(!std::is_same_v<T, Unit>);
    return Schedule<E>([result = Result<T, E>{std::forward<Args>(args)...}]() mutable {
      return std::move(result);
    });
  }
}

}  // namespace yaclib
