#pragma once

#include <cstdint>

namespace yaclib {
namespace detail {

class Injector;

}  // namespace detail

detail::Injector* GetInjector() noexcept;

void InjectFault() noexcept;

std::uint64_t GetInjectedCount() noexcept;

}  // namespace yaclib

#define YACLIB_INJECT_FAULT(statement)                                                                                 \
  yaclib::InjectFault();                                                                                               \
  statement;                                                                                                           \
  yaclib::InjectFault()
