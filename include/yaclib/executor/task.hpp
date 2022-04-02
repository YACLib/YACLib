#pragma once

#include <yaclib/config.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/func.hpp>

namespace yaclib {

/**
 * Callable that can be executed in an IExecutor \see IExecutor
 */
class ITask : public IFunc, public detail::Node {
 public:
  virtual void Cancel() noexcept = 0;
};

}  // namespace yaclib
