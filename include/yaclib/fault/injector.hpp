#pragma once

#include <atomic>

namespace yaclib::detail {

// TODO(myannyax) Add metrics, refactor this shit
class Injector final {
 public:
  explicit Injector() noexcept = default;

  void MaybeInject() noexcept;

  uint32_t GetState() const noexcept;

  void SetState(uint32_t state) noexcept;

  void Disable() noexcept;

  void Enable() noexcept;

  static void SetFrequency(uint32_t freq) noexcept;
  static void SetSleepTime(uint32_t ns) noexcept;

  static uint32_t GetSleepTime() noexcept;

  static uint64_t GetInjectedCount() noexcept;

 private:
  bool NeedInject() noexcept;
  void Reset() noexcept;

  static uint32_t sYieldFrequency;
  static uint32_t sSleepTime;
  static uint64_t sInjectedCount;

  std::atomic_uint32_t _count{0};
  bool _pause{false};
};

}  // namespace yaclib::detail
