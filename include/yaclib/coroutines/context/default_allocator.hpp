#pragma once
#include <yaclib/coroutines/context/stack_allocator.hpp>

#include <vector>

namespace yaclib::coroutines {

// probably won't work for win
class DefaultAllocator : public StackAllocator {
 public:
  [[nodiscard]] Allocation Allocate() const override;

  void Release(Allocation allocation) override;

  void SetMinStackSize(size_t bytes) override;

  size_t GetMinStackSize() override;

 private:
  size_t _stack_size_pages = 2;
  std::vector<Allocation> _pool;
};

extern DefaultAllocator default_allocator_instance;

}  // namespace yaclib::coroutines
