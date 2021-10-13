#pragma once
#include <cstddef>

/***
 * passed to coroutine constructor
 */

struct Allocation {
  char* start;
  size_t size;
};

class StackAllocator {
 public:
  [[nodiscard]] virtual Allocation Allocate() const = 0;
  virtual void Release(Allocation) = 0;
  virtual void SetMinStackSize(size_t bytes) = 0;
  virtual ~StackAllocator() = default;
};