#pragma once

#include <yaclib/util/detail/default_deleter.hpp>

namespace yaclib::detail {

template <typename CounterBase, typename Deleter = DefaultDeleter>
struct OneCounter : CounterBase {
  template <typename... Args>
  OneCounter(std::size_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<CounterBase, Args&&...>)
    : CounterBase{std::forward<Args>(args)...} {
  }

  constexpr bool SubEqual(std::size_t) const {
    return false;
  }

  void Add(std::size_t) noexcept {  // LCOV_EXCL_LINE compiler remove this call from tests
  }                                 // LCOV_EXCL_LINE

  void Sub(std::size_t) noexcept {
    Deleter::Delete(*this);
  }
};

}  // namespace yaclib::detail
