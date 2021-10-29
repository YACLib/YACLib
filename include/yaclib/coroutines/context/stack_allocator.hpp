#pragma once
#include <cstddef>

namespace yaclib {

struct Allocation {
  char* start = nullptr;
  size_t size = 0;
};

/***
 * passed to coroutine constructor
 */
class StackAllocator {
 public:
  [[nodiscard]] virtual Allocation Allocate() const = 0;

  virtual void Release(Allocation) = 0;

  virtual void SetMinStackSize(size_t bytes) = 0;

  virtual size_t GetMinStackSize() = 0;

  virtual ~StackAllocator() = default;
};

}  // namespace yaclib
