#include <fault/util.hpp>

#include <yaclib/fault/detail/atomic.hpp>

#include <atomic>

namespace yaclib::detail {

static std::atomic_uint32_t sAtomicFailFrequency = 13;

bool ShouldFailAtomicWeak() {
  auto freq = sAtomicFailFrequency.load(std::memory_order_relaxed);
  return freq != 0 && GetRandNumber(freq) == 0;
}

void SetAtomicWeakFailFrequency(uint32_t k) {
  sAtomicFailFrequency = k;
}

}  // namespace yaclib::detail
