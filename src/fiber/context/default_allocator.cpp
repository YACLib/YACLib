#include <yaclib/fiber/default_allocator.hpp>

#include <sys/mman.h>

namespace yaclib {

// TODO(myannyax) change to getting actual page size
static const size_t kPageSize = 4096;

static size_t PagesToBytes(size_t count) {
  return count * kPageSize;
}

static void ProtectPages(char* start, size_t offset, size_t count) {
  mprotect(/*addr=*/static_cast<void*>(start + PagesToBytes(offset)),
           /*len=*/PagesToBytes(count),
           /*prot=*/PROT_NONE);
  // TODO(myannyax) check returns not -1
}

Allocation DefaultAllocator::Allocate() {
  // TODO(myannyax) make _pool not just a vector
  if (!_pool.empty()) {
    auto allocation = _pool.back();
    _pool.pop_back();
    return allocation;
  }
  size_t size = PagesToBytes(_stack_size_pages);

  void* start = mmap(/*addr=*/nullptr, /*length=*/size,
                     /*prot=*/PROT_READ | PROT_WRITE,
                     /*flags=*/MAP_PRIVATE | MAP_ANONYMOUS,
                     /*fd=*/-1, /*offset=*/0);

  // TODO(myannyax) check start != MAP_FAILED
  auto allocation = Allocation{(char*)start, size};
  ProtectPages(allocation.start, 0, 1);
  return allocation;
}

void DefaultAllocator::Release(Allocation allocation) {
  if (allocation.size == PagesToBytes(_stack_size_pages)) {
    _pool.push_back(allocation);
  } else {
    if (allocation.start == nullptr) {
      return;
    }

    munmap(static_cast<void*>(allocation.start), allocation.size);
    // TODO(myannyax) check returns not -1
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

DefaultAllocator gDefaultAllocator;

}  // namespace yaclib
