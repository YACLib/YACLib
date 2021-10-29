#pragma once
#include <yaclib/coroutines/context/stack_allocator.hpp>
#include <yaclib/coroutines/context/stack_view.hpp>

#include <cstddef>

namespace yaclib {

class Stack {
 public:
  Stack(Allocation allocation, StackAllocator& allocator) : _allocation(allocation), _allocator(allocator) {
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

  [[nodiscard]] StackAllocator& GetAllocator() const {
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
  StackAllocator& _allocator;
};

}  // namespace yaclib
