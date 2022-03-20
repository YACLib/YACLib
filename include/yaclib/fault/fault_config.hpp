#pragma once

#include <cstdint>

#ifdef YACLIB_FAULT
#  include <yaclib/fault/detail/yielder.hpp>
#endif

/**
 * Sets frequency with which fault will be injected, default is 16.
 */
void SetFrequency(uint32_t freq);

/**
 * Sets sleep time if sleep is used instead of yield for interrupting thread execution for fault injection, default is
 * 200
 */
void SetSleepTime(uint32_t ns);
