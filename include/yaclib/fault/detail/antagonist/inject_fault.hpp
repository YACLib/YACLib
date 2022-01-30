#pragma once

#include <yaclib/fault/detail/antagonist/yielder.hpp>

#define YACLIB_INJECT_FAULT(statement)                                                                                 \
  yaclib::detail::InjectFault();                                                                                       \
  statement;                                                                                                           \
  yaclib::detail::InjectFault()

namespace yaclib::detail {

Yielder* GetYielder();

void InjectFault();

}  // namespace yaclib::detail
