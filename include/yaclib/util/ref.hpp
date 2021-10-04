#pragma once

namespace yaclib::util {

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

  /**
   * Decrements reference counter without dtor
   */
  virtual void DecRefRelease() noexcept {};

  virtual ~IRef() = default;
};

}  // namespace yaclib::util
