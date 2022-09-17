#pragma once

#include <yaclib/util/ref.hpp>

#include <type_traits>
#include <utility>

namespace yaclib {

/**
 * Callable interface
 */
class IFunc : public IRef {  // TODO(MBkkt) Maybe remove inheritance from IRef
 public:
  // compiler remove this call from tests
  virtual void Call() noexcept {  // LCOV_EXCL_LINE
    YACLIB_PURE_VIRTUAL();        // LCOV_EXCL_LINE
  }                               // LCOV_EXCL_LINE
};

}  // namespace yaclib
