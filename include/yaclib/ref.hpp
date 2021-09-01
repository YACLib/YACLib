#pragma once

namespace yaclib {

/**
 * \class Reference counting interface
 */
class IRef {
 public:
  /**
   * \brief Increments reference counter
   */
  virtual void IncRef() noexcept = 0;

  /**
   * \brief Decrements reference counter
   */
  virtual void DecRef() noexcept = 0;

  virtual ~IRef() = default;
};

}  // namespace yaclib
