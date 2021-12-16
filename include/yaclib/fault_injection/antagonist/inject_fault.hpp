#pragma once

#include "yielder.hpp"

namespace yaclib::detail {

Yielder* GetYielder();

void InjectFault();

}  // namespace yaclib::detail
