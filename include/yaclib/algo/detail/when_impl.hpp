#pragma once

#include <yaclib/async/future.hpp>

#include <cstddef>
#include <utility>

namespace yaclib::detail {

template <typename Combinator, typename It>
void WhenImpl(Combinator* combinator, It it, std::size_t count) {
  for (std::size_t i = 0; i != count; ++i) {
    auto core = std::exchange(it->GetCore(), nullptr);
    core->SetCallbackInline(*combinator);
    ++it;
  }
}

template <typename Combinator, typename E, typename... V>
void WhenImpl(Combinator* combinator, Future<V, E>&&... futures) {
  // TODO(MBkkt) Make Impl for BaseCore's instead of futures
  (..., std::exchange(futures.GetCore(), nullptr)->SetCallbackInline(*combinator));
}

}  // namespace yaclib::detail
