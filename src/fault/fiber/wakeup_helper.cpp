#include <yaclib/fault/detail/fiber/fiber_base.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/fault/detail/fiber/wakeup_helper.hpp>

namespace yaclib::detail::fiber {

void ScheduleFiber(FiberBase* fiber) {
  yaclib::fault::Scheduler::GetScheduler()->Schedule(fiber);
}

}  // namespace yaclib::detail::fiber
