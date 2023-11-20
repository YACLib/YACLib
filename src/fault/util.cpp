#include <fault/util.hpp>

namespace yaclib::detail {

static std::uint32_t sSeed = 1239;

#if YACLIB_FAULT == 2
static std::uint64_t sRandCount = 0;
#endif

static thread_local std::mt19937_64 eng{sSeed};

void SetSeed(std::uint32_t new_seed) {
  sSeed = new_seed;
  eng.seed(new_seed);
}

std::uint32_t GetSeed() {
  return sSeed;
}

std::uint64_t GetRandNumber(std::uint64_t max) {
#if YACLIB_FAULT == 2
  sRandCount++;
#endif
  return eng() % max;
}

std::uint64_t GetRandCount() {
#if YACLIB_FAULT == 2
  return sRandCount;
#else
  return 0;
#endif
}

void ForwardToRandCount([[maybe_unused]] std::uint64_t random_count) {
#if YACLIB_FAULT == 2
  for (std::uint64_t i = 0; i != random_count; ++i) {
    GetRandNumber(1);
  }
#endif
}

}  // namespace yaclib::detail
