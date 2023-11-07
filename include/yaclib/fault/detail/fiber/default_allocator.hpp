#pragma once

#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/log.hpp>

#include <cstdint>
#include <vector>

namespace yaclib::detail::fiber {

/**
 * Allocator used by default
 */
class DefaultAllocator final : public IStackAllocator {
 public:
  [[nodiscard]] Allocation Allocate() final;

  void Release(Allocation allocation) final;

  void SetMinStackSize(std::size_t pages) noexcept final;

  std::size_t GetMinStackSize() noexcept final;

  static void SetCacheSize(std::uint32_t size) noexcept;

 private:
  std::vector<Allocation> _pool;
  std::size_t _stack_size_pages{8};
};

}  // namespace yaclib::detail::fiber
