#include <fault/util.hpp>

#include <yaclib/fault/injector.hpp>

#include <yaclib_std/thread>

namespace yaclib::detail {

std::uint32_t Injector::sYieldFrequency = 16;
std::uint32_t Injector::sSleepTime = 100;
std::uint64_t Injector::sInjectedCount = 0;

void Injector::MaybeInject() noexcept {
  if (NeedInject()) {
#if YACLIB_FAULT == 2
    yaclib_std::this_thread::yield();
    sInjectedCount += 1;
#elif defined(_MSC_VER)
    yaclib_std::this_thread::yield();
#else
    yaclib_std::this_thread::sleep_for(std::chrono::nanoseconds(1 + GetRandNumber(sSleepTime)));
#endif
  }
}

bool Injector::NeedInject() noexcept {
  if (_pause) {
    return false;
  }
  if (++_count >= sYieldFrequency) {
    Reset();
    return true;
  }
  return false;
}

void Injector::Reset() noexcept {
  _count = static_cast<std::uint32_t>(GetRandNumber(sYieldFrequency));
}

void Injector::SetFrequency(std::uint32_t freq) noexcept {
  sYieldFrequency = freq;
}

void Injector::SetSleepTime(std::uint32_t ns) noexcept {
  sSleepTime = ns;
}

std::uint32_t Injector::GetSleepTime() noexcept {
  return sSleepTime;
}

std::uint64_t Injector::GetInjectedCount() noexcept {
  return sInjectedCount;
}

std::uint32_t Injector::GetState() const noexcept {
  return _count;
}

void Injector::SetState(std::uint32_t state) noexcept {
  _count = state;
}

void Injector::Disable() noexcept {
  _pause = true;
}

void Injector::Enable() noexcept {
  _pause = false;
}

}  // namespace yaclib::detail
