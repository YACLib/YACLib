#pragma once

#include <yaclib/fault/detail/fiber/stack_allocator.hpp>

#include <cstddef>

namespace yaclib::detail {
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

  ~Stack() {
    _allocator.Release(_allocation);
  }

 private:
  Allocation _allocation;
  IStackAllocator& _allocator;
};

}  // namespace yaclib::detail
