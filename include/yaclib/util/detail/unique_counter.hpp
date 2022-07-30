#pragma once

#include <yaclib/util/detail/default_deleter.hpp>

namespace yaclib::detail {

template <typename CounterBase, typename Deleter = DefaultDeleter>
struct UniqueCounter : CounterBase {
  using CounterBase::CounterBase;

  void IncRef() noexcept final {  // LCOV_EXCL_LINE compiler remove this call from tests
  }                               // LCOV_EXCL_LINE

  void DecRef() noexcept final {
    Deleter::Delete(*this);
  }
};

}  // namespace yaclib::detail
