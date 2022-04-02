#pragma once

#include <yaclib/config.hpp>

namespace yaclib {

/**
 * Reference counting interface
 */
class IRef {
 public:
  /**
   * Increments reference counter
   */
  virtual void IncRef() noexcept = 0;

  /**
   * Decrements reference counter
   */
  virtual void DecRef() noexcept = 0;

  virtual ~IRef() = default;
};

}  // namespace yaclib
