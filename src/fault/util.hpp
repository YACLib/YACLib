#include <atomic>
#include <random>

namespace yaclib::detail {

void SetSeed(uint32_t new_seed);

uint32_t GetSeed();

uint64_t GetRandNumber(uint64_t max);

}  // namespace yaclib::detail
