#pragma once

namespace yaclib {

void InjectFault() noexcept;

}  // namespace yaclib

#define YACLIB_FAULT_INJECT(statement)                                                                                 \
  yaclib::InjectFault();                                                                                               \
  statement;                                                                                                           \
  yaclib::InjectFault()
