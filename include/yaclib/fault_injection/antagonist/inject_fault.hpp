#pragma once

#include <yaclib/fault_injection/antagonist/yielder.hpp>

namespace yaclib::detail {

Yielder* GetYielder();

void InjectFault();

}  // namespace yaclib::detail
