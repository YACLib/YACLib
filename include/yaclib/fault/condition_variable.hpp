#pragma once

#include <yaclib/fault/detail/condition/condition_variable.hpp>

namespace yaclib_std {

#ifdef YACLIB_FAULT

using condition_variable = yaclib::detail::ConditionVariable;

#else

using condition_variable = std::condition_variable;

#endif
}  // namespace yaclib_std
