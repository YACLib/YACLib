#pragma once

#include <yaclib/algo/detail/unique_core.hpp>
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
 * \tparam T trait of the Future, by default \ref DefaultTrait
 * \tparam Args if single, and V default, then used as type of Future value
 * \param args for fulfill Future
 * \return Ready Future
 */
template <typename V = Unit, typename T = DefaultTrait, typename... Args>
/*Future*/ auto MakeFuture(Args&&... args) {
  if constexpr (sizeof...(Args) == 0) {
    using Value = std::conditional_t<std::is_same_v<V, Unit>, void, V>;
    return Future{detail::UniqueCorePtr<Value, T>{MakeUnique<detail::UniqueCore<Value, T>>(std::in_place)}};
  } else if constexpr (std::is_same_v<V, Unit>) {
    using Head = std::decay_t<head_t<Args&&...>>;
    using Value = std::conditional_t<std::is_same_v<Head, Unit>, void, Head>;
    return Future{detail::UniqueCorePtr<Value, T>{
      MakeUnique<detail::UniqueCore<Value, T>>(std::in_place, std::forward<Args>(args)...)}};
  } else {
    return Future{
      detail::UniqueCorePtr<V, T>{MakeUnique<detail::UniqueCore<V, T>>(std::in_place, std::forward<Args>(args)...)}};
  }
}

}  // namespace yaclib
