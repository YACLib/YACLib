#pragma once

namespace yaclib::detail::fiber {

class FiberBase;

void ScheduleFiber(FiberBase* fiber);

}  // namespace yaclib::detail::fiber
