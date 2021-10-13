#pragma once
#include "stack_allocator.h"

#include <vector>

// probably won't work for win
class DefaultAllocator : public StackAllocator {
  [[nodiscard]] Allocation Allocate() const override;
  void Release(Allocation allocation) override;
  void SetMinStackSize(size_t bytes) override;

 private:
  size_t _stack_size_pages;
  std::vector<Allocation> _pool;
};
