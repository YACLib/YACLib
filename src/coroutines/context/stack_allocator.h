#pragma once

#include "stack.h"

#include <cstddef>

/***
 * passed to coroutine constructor
 */

struct Allocation {
  char* start;
  size_t size;
};

class Stack;

class StackAllocator {
 public:
  [[nodiscard]] virtual Stack Allocate() const = 0;
  virtual void Release(Allocation) const = 0;
  virtual void SetMinStackSize(size_t bytes) = 0;
  virtual ~StackAllocator() = default;
};