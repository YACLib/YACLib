#include <fault/util.hpp>

#include <yaclib/fault/config.hpp>
#include <yaclib/fault/injector.hpp>
#if YACLIB_FAULT != 0
#  include <yaclib/fault/detail/atomic.hpp>
#endif
#if YACLIB_FAULT == 2
#  include <yaclib/fault/detail/fiber/scheduler.hpp>
#endif

namespace yaclib {

void SetFaultFrequency(std::uint32_t freq) {
  yaclib::detail::Injector::SetFrequency(freq);
}

void SetFaultSleepTime(std::uint32_t ns) {
  yaclib::detail::Injector::SetSleepTime(ns);
}

void SetAtomicFailFrequency(std::uint32_t k) {
#if YACLIB_FAULT != 0
  yaclib::detail::SetAtomicWeakFailFrequency(k);
#endif
}

void SetSeed(std::uint32_t seed) {
  detail::SetSeed(seed);
}

void SetFaultTickLength(std::uint32_t ns) {
#if YACLIB_FAULT == 2
  fault::Scheduler::SetTickLength(ns);
#endif
}

void SetFaultRandomListPick(std::uint32_t k) {
#if YACLIB_FAULT == 2
  fault::Scheduler::SetRandomListPick(k);
#endif
}

void SetFaultFiberStackSize(std::uint32_t pages) {
#if YACLIB_FAULT == 2
  yaclib::detail::fiber::FiberBase::GetAllocator().SetMinStackSize(pages);
#endif
}

void SetFaultFiberStackCacheSize(std::uint32_t c) {
#if YACLIB_FAULT == 2
  yaclib::detail::fiber::DefaultAllocator::SetCacheSize(c);
#endif
}

void SetFaultHardwareConcurrency(std::uint32_t c) {
  // TODO(myannyax): set for thread based fault too
#if YACLIB_FAULT == 2
  yaclib::detail::fiber::Thread::SetHardwareConcurrency(c);
#endif
}

uint64_t GetFaultRandomCount() {
  return detail::GetRandCount();
}

void ForwardToFaultRandomCount(uint64_t random_count) {
  return detail::ForwardToRandCount(random_count);
}

uint32_t GetInjectorState() {
#if YACLIB_FAULT != 0
  return yaclib::GetInjector()->GetState();
#else
  return 0;
#endif
}

void SetInjectorState(uint32_t state) {
#if YACLIB_FAULT != 0
  yaclib::GetInjector()->SetState(state);
#endif
}

}  // namespace yaclib
