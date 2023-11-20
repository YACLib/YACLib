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
  detail::Injector::SetFrequency(freq);
}

void SetFaultSleepTime(std::uint32_t ns) noexcept {
  detail::Injector::SetSleepTime(ns);
}

std::uint32_t GetFaultSleepTime() noexcept {
  return detail::Injector::GetSleepTime();
}

void SetAtomicFailFrequency([[maybe_unused]] std::uint32_t k) noexcept {
#if YACLIB_FAULT != 0
  detail::SetAtomicWeakFailFrequency(k);
#endif
}

void SetSeed(std::uint32_t seed) noexcept {
  detail::SetSeed(seed);
}

namespace fiber {

void SetFaultTickLength([[maybe_unused]] std::uint32_t ns) noexcept {
#if YACLIB_FAULT == 2
  fault::Scheduler::SetTickLength(ns);
#endif
}

void SetFaultRandomListPick([[maybe_unused]] std::uint32_t k) noexcept {
#if YACLIB_FAULT == 2
  detail::fiber::SetRandomListPick(k);
#endif
}

void SetStackSize([[maybe_unused]] std::uint32_t pages) noexcept {
#if YACLIB_FAULT == 2
  detail::fiber::FiberBase::GetAllocator().SetMinStackSize(pages);
#endif
}

void SetStackCacheSize([[maybe_unused]] std::uint32_t c) noexcept {
#if YACLIB_FAULT == 2
  detail::fiber::DefaultAllocator::SetCacheSize(c);
#endif
}

void SetHardwareConcurrency([[maybe_unused]] std::uint32_t c) noexcept {
#if YACLIB_FAULT == 2
  detail::fiber::Thread::SetHardwareConcurrency(c);
#endif
}

std::uint64_t GetFaultRandomCount() noexcept {
  return detail::GetRandCount();
}

void ForwardToFaultRandomCount(std::uint64_t random_count) noexcept {
  return detail::ForwardToRandCount(random_count);
}

std::uint32_t GetInjectorState() noexcept {
#if YACLIB_FAULT != 0
  return GetInjector()->GetState();
#else
  return 0;
#endif
}

void SetInjectorState([[maybe_unused]] std::uint32_t state) noexcept {
#if YACLIB_FAULT != 0
  GetInjector()->SetState(state);
#endif
}

}  // namespace fiber
}  // namespace yaclib
