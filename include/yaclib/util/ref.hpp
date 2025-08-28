#pragma once

#include <yaclib/log.hpp>

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

  // The base implementations are ok only if the object
  // is not actually reference counted
  // If you need GetRef() then you must implement it all
  virtual std::size_t GetRef() noexcept {  // LCOV_EXCL_LINE
    YACLIB_PURE_VIRTUAL();                 // LCOV_EXCL_LINE
    return -1;                             // LCOV_EXCL_LINE
  }  // LCOV_EXCL_LINE

  virtual ~IRef() noexcept = default;
};

}  // namespace yaclib
