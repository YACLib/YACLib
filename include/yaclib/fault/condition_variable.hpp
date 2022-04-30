#pragma once

#ifdef YACLIB_FAULT

#  ifdef YACLIB_FIBER

#    include <yaclib/fault/detail/condition/fiber_condition_variable.hpp>

namespace yaclib_std {

using condition_variable = yaclib::detail::FiberConditionVariable;

}  // namespace yaclib_std

#  else

#    include <yaclib/fault/detail/condition/condition_variable.hpp>

namespace yaclib_std {

using condition_variable = yaclib::detail::ConditionVariable;

}  // namespace yaclib_std

#  endif

#else

#  include <condition_variable>

namespace yaclib_std {

using condition_variable = std::condition_variable;

}  // namespace yaclib_std

#endif
