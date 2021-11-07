#pragma once

#include <yaclib/fiber/stack_allocator.hpp>
#include <yaclib/fiber/stack_view.hpp>

#include <cstddef>

namespace yaclib {
/**
 * Manages stack memory
 */
class Stack {
 public:
  explicit Stack(IStackAllocator& allocator) : _allocation(allocator.Allocate()), _allocator(allocator) {
  }

  Stack(Stack&& that) = default;

  Stack& operator=(Stack&& other) noexcept {
    auto& new_allocation = other.GetAllocation();
    _allocator.Release(_allocation);
    _allocation = other.GetAllocation();
    _allocator = other.GetAllocator();
    new_allocation.size = 0;
    new_allocation.start = nullptr;
    return *this;
  }

  [[nodiscard]] Allocation& GetAllocation() {
    return _allocation;
  }

  [[nodiscard]] IStackAllocator& GetAllocator() const {
    return _allocator;
  }

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
  IStackAllocator& _allocator;
};

}  // namespace yaclib
