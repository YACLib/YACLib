#pragma once
#include "stack_allocator.h"
#include "stack_view.h"
#include <cstddef>

class Stack {
 public:
  Stack(Allocation allocation, StackAllocator& allocator) : _allocation(allocation), _allocator(allocator) {
  }

  Stack(Stack&& that) = default;

  [[nodiscard]] size_t Size() const {
    return _allocation.size;
  }

  [[nodiscard]] StackView View() const {
    return {_allocation.start, _allocation.size};
  }

  ~Stack() {
    _allocator.Release(_allocation);
  }

 private:
  Allocation _allocation;
  StackAllocator& _allocator;
};
