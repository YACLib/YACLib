#pragma once

// TODO(myannayx): define in cmake depending on system

#include "yaclib/fault/thread.hpp"

#include <atomic>
#include <random>

namespace yaclib::detail {

// TODO(myannyax) stats?
class Yielder {
 public:
  explicit Yielder();
  void MaybeYield();

  static void SetFrequency(uint32_t freq);

  static void SetSleepTime(uint32_t ns);

 private:
  bool ShouldYield();
  void Reset();
  uint32_t RandNumber(uint32_t max);

  static std::atomic_uint32_t yield_frequency;
  static std::atomic_uint32_t sleep_time;

  uint32_t _count;
  std::mt19937 _eng;
};

}  // namespace yaclib::detail
