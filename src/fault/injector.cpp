#include <fault/util.hpp>

#include <yaclib/fault/injector.hpp>

#include <yaclib_std/thread>

namespace yaclib::detail {

static std::uint32_t sYieldFrequency = 16;
static std::uint32_t sSleepTime = 100;
static std::uint64_t sInjectedCount = 0;

void Injector::MaybeInject() noexcept {
  if (NeedInject()) {
#if YACLIB_FAULT == 2
    ++sInjectedCount;
    yaclib_std::this_thread::yield();
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
  if (_count.fetch_add(1, std::memory_order_relaxed) >= sYieldFrequency) {
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
  return _count.load(std::memory_order_relaxed);
}

void Injector::SetState(std::uint32_t state) noexcept {
  _count.store(state, std::memory_order_relaxed);
}

void Injector::Disable() noexcept {
  _pause = true;
}

void Injector::Enable() noexcept {
  _pause = false;
}

}  // namespace yaclib::detail
