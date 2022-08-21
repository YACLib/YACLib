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
auto MakeFuture(Args&&... args) {
  if constexpr (sizeof...(Args) == 0) {
    return Future{MakeUnique<detail::ResultCore<void, E>>(Unit{})};
  } else if constexpr (sizeof...(Args) == 1) {
    using T = std::conditional_t<std::is_same_v<V, Unit>, head_t<Args...>, V>;
    static_assert(!std::is_same_v<T, Unit>);
    return Future{MakeUnique<detail::ResultCore<T, E>>(std::forward<Args>(args)...)};
  } else {
    static_assert(!std::is_same_v<V, Unit>);
    return Future{MakeUnique<detail::ResultCore<V, E>>(std::forward<Args>(args)...)};
  }
}

}  // namespace yaclib
