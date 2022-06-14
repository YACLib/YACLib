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

  void Start();

  void SwitchTo(ExecutionContext& other);

  void Exit(ExecutionContext& other);

 private:
  Allocation _stack;
  ucontext_t _context;

#ifdef YACLIB_ASAN
  const void* _old_stack_bottom{nullptr};
  size_t _old_stack_size{0};
#endif
};

}  // namespace yaclib::detail::fiber
