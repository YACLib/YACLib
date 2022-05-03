#include <yaclib/fault/detail/fiber/default_allocator.hpp>

#include <sys/mman.h>
#include <unistd.h>

namespace yaclib::detail::fiber {

static const uint32_t kPageSize = sysconf(_SC_PAGESIZE);

static uint32_t cache_size = 100;

Allocation DefaultAllocator::Allocate() {
  if (!_pool.empty()) {
    auto allocation = _pool.back();
    _pool.pop_back();
    return allocation;
  }
  size_t size = _stack_size_pages * kPageSize;

  void* start = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  YACLIB_ERROR(start == MAP_FAILED, "mmap for stack failed");
  auto status = mprotect(static_cast<void*>(start), kPageSize, PROT_NONE);
  YACLIB_ERROR(status == -1, "mprotect for stack failed");

  auto allocation = Allocation{static_cast<char*>(start), size};
  return allocation;
}

void DefaultAllocator::Release(Allocation allocation) {
  if (_pool.size() < cache_size) {
    _pool.push_back(allocation);
  } else {
    if (allocation.start == nullptr) {
      return;
    }

    auto status = munmap(static_cast<void*>(allocation.start), allocation.size);
    YACLIB_ERROR(status == -1, "munmap for stack failed");
  }
}

void DefaultAllocator::SetMinStackSize(size_t pages) {
  if (pages > _stack_size_pages) {
    for (auto& allocation : _pool) {
      Release(allocation);
    }
    _pool.clear();
    _stack_size_pages = pages + 1;
  }
}

size_t DefaultAllocator::GetMinStackSize() {
  return _stack_size_pages;
}

void DefaultAllocator::SetCacheSize(uint32_t size) {
  cache_size = size;
}

}  // namespace yaclib::detail::fiber
