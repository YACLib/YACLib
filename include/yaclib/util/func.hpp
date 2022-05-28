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
  virtual void Call() noexcept = 0;
};

}  // namespace yaclib
