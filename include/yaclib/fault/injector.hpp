#pragma once

#include <atomic>
#include <random>

namespace yaclib::detail {

// TODO(myannyax) Add metrics, refactor this shit
class Injector {
 public:
  explicit Injector();
  void MaybeInject();

  static void SetFrequency(uint32_t freq);
  static void SetSleepTime(uint32_t ns);

 private:
  bool NeedInject();
  void Reset();
  uint32_t RandNumber(uint32_t max);

  static std::atomic_uint32_t yield_frequency;
  static std::atomic_uint32_t sleep_time;

  uint32_t _count;
  std::mt19937 _eng;
};

}  // namespace yaclib::detail
