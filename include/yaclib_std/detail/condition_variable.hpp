#pragma once

#if YACLIB_FAULT_CONDITION_VARIABLE == 2
#  include <yaclib/fault/detail/condition_variable.hpp>
#  include <yaclib/fault/detail/fiber/condition_variable.hpp>

namespace yaclib_std {

using condition_variable = yaclib::detail::ConditionVariable<yaclib::detail::fiber::ConditionVariable>;

}  // namespace yaclib_std
#elif YACLIB_FAULT_CONDITION_VARIABLE == 1
#  include <yaclib/fault/detail/condition_variable.hpp>

#  include <condition_variable>

namespace yaclib_std {

using condition_variable = yaclib::detail::ConditionVariable<std::condition_variable>;

}  // namespace yaclib_std
#else
#  include <condition_variable>

namespace yaclib_std {

using condition_variable = std::condition_variable;

}  // namespace yaclib_std
#endif
