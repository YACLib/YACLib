#pragma once

#include <yaclib/log.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/func.hpp>

namespace yaclib {

/**
 * Callable that can be executed in an IExecutor \see IExecutor
 */
class Job : public IFunc, public detail::Node {
 public:
  virtual void Drop() noexcept {  // LCOV_EXCL_LINE  compiler remove this call from tests
    YACLIB_PURE_VIRTUAL();        // LCOV_EXCL_LINE
  }                               // LCOV_EXCL_LINE
};

}  // namespace yaclib
