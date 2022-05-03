#include <fault/util.hpp>

namespace yaclib::detail {

inline constexpr int kSeed = 1239;

static std::atomic_uint32_t seed = kSeed;

#if YACLIB_FAULT == 2
static int rand_count;
#endif

// TODO(myannyax): make not thread_static
thread_local static std::mt19937_64 eng(seed.load());

void SetSeed(uint32_t new_seed) {
  seed = new_seed;
  eng.seed(new_seed);
}

uint32_t GetSeed() {
  return seed;
}

uint64_t GetRandNumber(uint64_t max) {
#if YACLIB_FAULT == 2
  rand_count++;
#endif
  return eng() % max;
}

uint64_t GetRandCount() {
#if YACLIB_FAULT == 2
  return rand_count;
#else
  return 0;
#endif
}

void ForwardToRandCount(uint64_t random_count) {
#if YACLIB_FAULT == 2
  for (int i = 0; i < rand_count; ++i) {
    GetRandNumber(1);
  }
#endif
}

}  // namespace yaclib::detail
