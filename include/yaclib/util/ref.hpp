#pragma once

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

  virtual ~IRef() noexcept = default;
};

}  // namespace yaclib
