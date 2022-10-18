#pragma once

#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/result.hpp>
#include <yaclib/util/type_traits.hpp>

#include <type_traits>
#include <utility>

namespace yaclib {

/**
 * Function for create Ready Future
 *
 * \tparam V if not default value, it's type of Future value
 * \tparam E type of Future error, by default its
 * \tparam Args if single, and V default, then used as type of Future value
 * \param args for fulfill Future
 * \return Ready Future
 */
template <typename V = Unit, typename E = StopError, typename... Args>
/*Future*/ auto MakeFuture(Args&&... args) {
  if constexpr (sizeof...(Args) == 0) {
    using T = std::conditional_t<std::is_same_v<V, Unit>, void, V>;
    return Future{detail::ResultCorePtr<T, E>{MakeUnique<detail::ResultCore<T, E>>(std::in_place)}};
  } else if constexpr (std::is_same_v<V, Unit>) {
    using T0 = std::decay_t<head_t<Args&&...>>;
    using T = std::conditional_t<std::is_same_v<T0, Unit>, void, T0>;
    return Future{
      detail::ResultCorePtr<T, E>{MakeUnique<detail::ResultCore<T, E>>(std::in_place, std::forward<Args>(args)...)}};
  } else {
    return Future{detail::ResultCorePtr<V, E>{MakeUnique<detail::ResultCore<V, E>>(std::forward<Args>(args)...)}};
  }
}

}  // namespace yaclib
