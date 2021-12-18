#pragma once

#include <yaclib/fault/detail/antagonist/yielder.hpp>

namespace yaclib::detail {

Yielder* GetYielder();

void InjectFault();

}  // namespace yaclib::detail
