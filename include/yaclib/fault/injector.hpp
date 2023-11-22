#pragma once

#include <atomic>
#include <cstdint>

namespace yaclib::detail {

// TODO(myannyax) Add metrics, refactor this shit
class Injector final {
 public:
  void MaybeInject() noexcept;

  std::uint32_t GetState() const noexcept;

  void SetState(std::uint32_t state) noexcept;

  void Disable() noexcept;

  void Enable() noexcept;

  static void SetFrequency(std::uint32_t freq) noexcept;
  static void SetSleepTime(std::uint32_t ns) noexcept;

  static std::uint32_t GetSleepTime() noexcept;

  static std::uint64_t GetInjectedCount() noexcept;

 private:
  bool NeedInject() noexcept;
  void Reset() noexcept;

  std::atomic_uint32_t _count{0};
  bool _pause{false};
};

}  // namespace yaclib::detail
