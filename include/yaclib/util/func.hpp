#pragma once

#include <yaclib/config.hpp>
#include <yaclib/util/ref.hpp>

#include <type_traits>
#include <utility>

namespace yaclib {

/**
 * Callable interface
 */
class IFunc : public IRef {
 public:
  virtual void Call() noexcept = 0;
};

}  // namespace yaclib
