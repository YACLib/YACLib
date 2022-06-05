#include <fault/util.hpp>

#include <yaclib/fault/config.hpp>
#include <yaclib/fault/injector.hpp>
#if YACLIB_FAULT != 0
#  include <yaclib/fault/detail/atomic.hpp>
#endif
#if YACLIB_FAULT == 2
#  include <yaclib/fault/detail/fiber/scheduler.hpp>
#  include <yaclib/fault/detail/fiber/thread.hpp>
#endif

namespace yaclib {

void SetFaultFrequency(std::uint32_t freq) noexcept {
  yaclib::detail::Injector::SetFrequency(freq);
}

void SetFaultSleepTime(std::uint32_t ns) noexcept {
  yaclib::detail::Injector::SetSleepTime(ns);
}

std::uint32_t GetFaultSleepTime() noexcept {
  return yaclib::detail::Injector::GetSleepTime();
}

void SetAtomicFailFrequency(std::uint32_t k) noexcept {
#if YACLIB_FAULT != 0
  yaclib::detail::SetAtomicWeakFailFrequency(k);
#endif
}

void SetSeed(std::uint32_t seed) noexcept {
  detail::SetSeed(seed);
}

void SetFaultTickLength(std::uint32_t ns) noexcept {
#if YACLIB_FAULT == 2
  fault::Scheduler::SetTickLength(ns);
#endif
}

void SetFaultRandomListPick(std::uint32_t k) noexcept {
#if YACLIB_FAULT == 2
  fault::Scheduler::SetRandomListPick(k);
#endif
}

namespace fiber {

void SetStackSize(std::uint32_t pages) noexcept {
#if YACLIB_FAULT == 2
  yaclib::detail::fiber::FiberBase::GetAllocator().SetMinStackSize(pages);
#endif
}

void SetStackCacheSize(std::uint32_t c) noexcept {
#if YACLIB_FAULT == 2
  yaclib::detail::fiber::DefaultAllocator::SetCacheSize(c);
#endif
}

void SetHardwareConcurrency(std::uint32_t c) noexcept {
#if YACLIB_FAULT == 2
  yaclib::detail::fiber::Thread::SetHardwareConcurrency(c);
#endif
}

uint64_t GetFaultRandomCount() noexcept {
  return detail::GetRandCount();
}

void ForwardToFaultRandomCount(uint64_t random_count) noexcept {
  return detail::ForwardToRandCount(random_count);
}

uint32_t GetInjectorState() noexcept {
#if YACLIB_FAULT != 0
  return yaclib::GetInjector()->GetState();
#else
  return 0;
#endif
}

void SetInjectorState(uint32_t state) noexcept {
#if YACLIB_FAULT != 0
  yaclib::GetInjector()->SetState(state);
#endif
}

}  // namespace fiber
}  // namespace yaclib
