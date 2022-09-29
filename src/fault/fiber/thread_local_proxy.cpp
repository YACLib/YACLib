#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/fault/detail/fiber/thread_local_proxy.hpp>

#include <unordered_map>

namespace yaclib::detail::fiber {

std::uint64_t sNextFreeIndex = 0;

static std::unordered_map<std::uint64_t, void*>& GetMap() {
  static std::unordered_map<std::uint64_t, void*> sDefaults;
  return sDefaults;
}

void* GetImpl(std::uint64_t i) {
  auto* fiber = fault::Scheduler::Current();
  return fiber->GetTls(i, GetMap());
}

void Set(void* new_value, std::uint64_t i) {
  auto* fiber = fault::Scheduler::Current();
  fiber->SetTls(i, new_value);
}

void SetDefault(void* new_value, std::uint64_t i) {
  GetMap()[i] = new_value;
}

}  // namespace yaclib::detail::fiber
