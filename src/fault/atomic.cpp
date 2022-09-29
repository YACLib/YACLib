#include <fault/util.hpp>

#include <yaclib/fault/detail/atomic.hpp>

#include <atomic>

namespace yaclib::detail {

static std::uint32_t sAtomicFailFrequency = 13;

bool ShouldFailAtomicWeak() {
  auto freq = sAtomicFailFrequency;
  return freq != 0 && GetRandNumber(freq) == 0;
}

void SetAtomicWeakFailFrequency(std::uint32_t k) {
  sAtomicFailFrequency = k;
}

}  // namespace yaclib::detail
