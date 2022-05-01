#pragma once

#include <yaclib/fault/thread.hpp>

#include <atomic>

namespace yaclib::detail {

// TODO(myannyax) Add metrics, refactor this shit
class Injector {
 public:
  explicit Injector();
  void MaybeInject();

  uint32_t GetState();

  void SetState(uint32_t state);

  static void SetFrequency(uint32_t freq);
  static void SetSleepTime(uint32_t ns);

 private:
  bool NeedInject();
  void Reset();

  static std::atomic_uint32_t yield_frequency;
  static std::atomic_uint32_t sleep_time;

  uint32_t _count;
};

}  // namespace yaclib::detail
