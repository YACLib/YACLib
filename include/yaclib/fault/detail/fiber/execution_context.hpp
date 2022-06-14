#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>

#include <ucontext.h>

#ifdef YACLIB_ASAN
#  include <sanitizer/asan_interface.h>
#endif

namespace yaclib::detail::fiber {

using Trampoline = void (*)(void* arg);

class ExecutionContext {
 public:
  void Setup(Allocation stack, Trampoline trampoline, void* arg);

  void SwitchTo(ExecutionContext& other);

  void Exit(ExecutionContext& other);

 private:
  Allocation _stack;
  ucontext_t _context;
};

}  // namespace yaclib::detail::fiber
