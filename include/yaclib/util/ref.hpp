#pragma once

#include <cstddef>

namespace yaclib {

/**
 * Reference counting interface
 */
class IRef {
 public:
  /**
   * Increments reference counter
   */
  virtual void IncRef() noexcept {
  }

  /**
   * Decrements reference counter
   */
  virtual void DecRef() noexcept {
  }

  virtual std::size_t GetRef() noexcept {
    return 1;
  }

  virtual ~IRef() noexcept = default;
};

}  // namespace yaclib
