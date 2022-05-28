#pragma once

#include <yaclib/async/future.hpp>

#include <cstddef>
#include <utility>

namespace yaclib::detail {

template <typename Combinator, typename It>
void WhenImpl(Combinator* combinator, It it, std::size_t count) noexcept {
  for (std::size_t i = 0; i != count; ++i) {
    it->GetCore().Release()->SetHere(*combinator, PCore::kHereCall);
    ++it;
  }
}

template <typename Combinator, typename E, typename... V>
void WhenImpl(Combinator* combinator, FutureBase<V, E>&&... futures) noexcept {
  // TODO(MBkkt) Make Impl for CCore's instead of futures
  (..., futures.GetCore().Release()->SetHere(*combinator, PCore::kHereCall));
}

}  // namespace yaclib::detail
