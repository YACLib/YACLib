#pragma once

#include <yaclib/algo/detail/inline_core.hpp>
#include <yaclib/async/future.hpp>

#include <cstddef>
#include <utility>

namespace yaclib::detail {

// Combinator should be pointer for case when count == 0
template <typename Combinator, typename It>
void WhenImpl(Combinator* combinator, It it, std::size_t count) noexcept {
  for (std::size_t i = 0; i != count; ++i) {
    YACLIB_ASSERT(it->Valid());
    combinator->AddInput(*it->GetCore().Release());
    ++it;
  }
}

template <typename Combinator, typename E, typename... V>
void WhenImpl(Combinator& combinator, FutureBase<V, E>&&... fs) noexcept {
  YACLIB_ASSERT(... && fs.Valid());
  // TODO(MBkkt) Make Impl for BaseCore's instead of futures
  (..., combinator.AddInput(*fs.GetCore().Release()));
}

template <bool SymmetricTransfer>
auto WhenSetResult(BaseCore* callback) {
  if constexpr (SymmetricTransfer) {
    return callback->template SetResult<true>();
  } else {
    Loop(callback, callback->template SetResult<false>());
    return Noop<false>();
  }
}

}  // namespace yaclib::detail
