#pragma once

#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/log.hpp>

#include <vector>

namespace yaclib::detail::fiber {

/**
 * Allocator used by default
 */
class DefaultAllocator final : public IStackAllocator {
 public:
  [[nodiscard]] Allocation Allocate() final;

  void Release(Allocation allocation) final;

  void SetMinStackSize(size_t pages) noexcept final;

  size_t GetMinStackSize() noexcept final;

  static void SetCacheSize(uint32_t size) noexcept;

 private:
  size_t _stack_size_pages{8};
  std::vector<Allocation> _pool;
};

}  // namespace yaclib::detail::fiber
