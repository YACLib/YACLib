#pragma once

#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/log.hpp>

#include <vector>

namespace yaclib::detail {

/**
 * Allocator used by default
 */
class DefaultAllocator final : public IStackAllocator {
 public:
  [[nodiscard]] Allocation Allocate() final;

  void Release(Allocation allocation) final;

  void SetMinStackSize(size_t bytes) final;

  size_t GetMinStackSize() final;

 private:
  size_t _stack_size_pages = 6;
  std::vector<Allocation> _pool;
};

extern thread_local DefaultAllocator gDefaultAllocator;

}  // namespace yaclib::detail
