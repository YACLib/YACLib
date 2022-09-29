#pragma once

#include <cstddef>

namespace yaclib::detail::fiber {

struct Allocation final {
  char* start = nullptr;
  std::size_t size = 0;
};

/**
 * Passed to coroutine/fiber constructor, specifies the way in which
 * memory for Stack is Allocated and Released
 * \see Stack
 */
class IStackAllocator {
 public:
  [[nodiscard]] virtual Allocation Allocate() = 0;

  virtual void Release(Allocation) = 0;

  virtual void SetMinStackSize(std::size_t bytes) = 0;

  virtual std::size_t GetMinStackSize() = 0;

  virtual ~IStackAllocator() = default;
};

}  // namespace yaclib::detail::fiber
