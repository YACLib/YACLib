#pragma once

#include <yaclib/config.hpp>

#ifdef YACLIB_FAULT

#  include <yaclib/fault/detail/condition/condition_variable.hpp>

namespace yaclib_std {

using condition_variable = yaclib::detail::ConditionVariable;

}  // namespace yaclib_std

#else

#  include <condition_variable>

namespace yaclib_std {

using condition_variable = std::condition_variable;

}  // namespace yaclib_std

#endif
