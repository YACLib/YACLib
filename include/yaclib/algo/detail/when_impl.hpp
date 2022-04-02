#pragma once

#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <utility>

namespace yaclib::detail {

template <typename Combinator, typename It>
void WhenImpl(Combinator* combinator, It it, std::size_t size) {
  for (std::size_t i = 0; i != size; ++i) {
    auto core = std::exchange(it->GetCore(), nullptr);
    core->SetCallbackInline(*combinator);
    ++it;
  }
}

template <typename Combinator, typename... V, typename... E>
void WhenImpl(Combinator* combinator, Future<V, E>&&... futures) {
  (..., std::exchange(futures.GetCore(), nullptr)->SetCallbackInline(*combinator));
}

}  // namespace yaclib::detail
