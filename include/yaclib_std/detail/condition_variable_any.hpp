#pragma once

#if YACLIB_FAULT_CONDITION_VARIABLE_ANY == 2  // TODO(myannyax) Implement
/*
#  error "YACLIB_FAULT=FIBER not implemented yet"

#  include <yaclib/fault/detail/condition_variable_any.hpp>
#  include <yaclib/fault/detail/fiber/condition_variable_any.hpp>

namespace yaclib_std {

using condition_variable_any = yaclib::detail::ConditionVariableAny<yaclib::detail::fiber::ConditionVariableAny>;

}  // namespace yaclib_std
 */
#elif YACLIB_FAULT_CONDITION_VARIABLE_ANY == 1
#  include <yaclib/fault/detail/condition_variable_any.hpp>

#  include <condition_variable>

namespace yaclib_std {

using condition_variable_any = yaclib::detail::ConditionVariableAny<std::condition_variable_any>;

}  // namespace yaclib_std
#else
#  include <condition_variable>

namespace yaclib_std {

using condition_variable_any = std::condition_variable_any;

}  // namespace yaclib_std
#endif
