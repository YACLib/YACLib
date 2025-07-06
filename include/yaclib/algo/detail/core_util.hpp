#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/algo/detail/result_core.hpp>
#include <yaclib/algo/detail/shared_core.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/cast.hpp>

namespace yaclib::detail {

template <typename V, typename E, bool Shared>
auto& CoreDownCast(InlineCore& core) {
  if constexpr (Shared) {
    return DownCast<SharedCore<V, E>>(core);
  } else {
    return DownCast<ResultCore<V, E>>(core);
  }
}

template <bool Shared, typename Core>
decltype(auto) ResultFromCore(Core& core) {
  if constexpr (Shared) {
    return core.Get();
  } else {
    return std::move(core.Get());
  }
}

template <typename V, typename E, bool Shared>
decltype(auto) ResultFromCore(InlineCore& core) {
  return ResultFromCore<Shared>(CoreDownCast<V, E, Shared>(core));
}

}  // namespace yaclib::detail
