#pragma once
#include <cstddef>

namespace yaclib::coroutines {

struct Allocation {
  char* start;
  size_t size;
};

/***
 * passed to coroutine constructor
 */
class StackAllocator {
 public:
  [[nodiscard]] virtual Allocation Allocate() const = 0;
  virtual void Release(Allocation) = 0;
  virtual void SetMinStackSize(size_t bytes) = 0;
  virtual ~StackAllocator() = default;
};

}  // namespace yaclib::coroutines
