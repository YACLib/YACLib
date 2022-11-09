#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/async/future.hpp>

#include <cstddef>
#include <utility>

namespace yaclib::detail {

template <typename Combinator, typename It>
void WhenImpl(Combinator& combinator, It it, std::size_t count) noexcept {
  for (std::size_t i = 0; i != count; ++i) {
    combinator.AddInput(*it->GetCore().Release());
    ++it;
  }
}

template <typename Combinator, typename E, typename... V>
void WhenImpl(Combinator& combinator, FutureBase<V, E>&&... futures) noexcept {
  // TODO(MBkkt) Make Impl for BaseCore's instead of futures
  (..., combinator.AddInput(*futures.GetCore().Release()));
}

}  // namespace yaclib::detail
