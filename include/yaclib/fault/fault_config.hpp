#pragma once

#include <cstdint>

#ifdef YACLIB_FAULT
#include <yaclib/fault/detail/antagonist/yielder.hpp>
#endif

void SetFrequency(uint32_t freq);

void SetSleepTime(uint32_t ns);
