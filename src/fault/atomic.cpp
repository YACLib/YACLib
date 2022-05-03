#include <fault/util.hpp>

#include <yaclib/fault/detail/atomic.hpp>

#include <atomic>

namespace yaclib::detail {

inline constexpr int kAtomicFailFreq = 2;

static std::atomic_uint32_t atomic_fail_frequency = kAtomicFailFreq;

bool ShouldFailAtomicWeak() {
  auto freq = atomic_fail_frequency.load();
  return freq != 0 && GetRandNumber(freq) == 0;
}

void SetAtomicWeakFailFrequency(uint32_t k) {
  atomic_fail_frequency = k;
}

}  // namespace yaclib::detail
