#pragma once

#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/func.hpp>

namespace yaclib {

/**
 * Callable that can be executed in an IExecutor \see IExecutor
 */
class Job : public IFunc, public detail::Node {
 public:
  virtual void Drop() noexcept = 0;
};

}  // namespace yaclib
