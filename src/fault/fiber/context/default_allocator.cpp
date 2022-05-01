#include <yaclib/fault/detail/fiber/default_allocator.hpp>

#include <sys/mman.h>

namespace yaclib::detail {

// TODO(myannyax) change to getting actual page size
static const size_t kPageSize = 4096;

static size_t PagesToBytes(size_t count) {
  return count * kPageSize;
}

static void ProtectStackPages(char* start) {
  auto status = mprotect(static_cast<void*>(start), PagesToBytes(1), PROT_NONE);
  YACLIB_ERROR(status == -1, "mprotect for stack failed");
}

Allocation DefaultAllocator::Allocate() {
  if (!_pool.empty()) {
    auto allocation = _pool.back();
    _pool.pop_back();
    return allocation;
  }
  size_t size = PagesToBytes(_stack_size_pages);

  void* start = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  YACLIB_ERROR(start == MAP_FAILED, "mmap for stack failed");
  auto allocation = Allocation{static_cast<char*>(start), size};
  ProtectStackPages(allocation.start);
  return allocation;
}

void DefaultAllocator::Release(Allocation allocation) {
  if (allocation.size == PagesToBytes(_stack_size_pages)) {
    _pool.push_back(allocation);
  } else {
    if (allocation.start == nullptr) {
      return;
    }

    auto status = munmap(static_cast<void*>(allocation.start), allocation.size);
    YACLIB_ERROR(status == -1, "munmap for stack failed");
  }
}

void DefaultAllocator::SetMinStackSize(size_t bytes) {
  size_t pages = bytes / kPageSize;
  if (bytes % kPageSize != 0) {
    ++pages;
  }
  if (pages > _stack_size_pages) {
    for (auto& allocation : _pool) {
      Release(allocation);
    }
    _pool.clear();
    _stack_size_pages = pages + 1;
  }
}

size_t DefaultAllocator::GetMinStackSize() {
  return (_stack_size_pages - 1) * kPageSize;
}

thread_local DefaultAllocator gDefaultAllocator;

}  // namespace yaclib::detail
