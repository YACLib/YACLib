#pragma once

#include <yaclib/fiber/detail/setup_stack_x64.hpp>
#include <yaclib/fiber/stack_view.hpp>

#include <algorithm>
#include <ucontext.h>

#include <ucontext.h>

namespace yaclib {

class ExecutionContext {
 public:
  void Setup(StackView stack, Trampoline trampoline, void* arg);

  void SwitchTo(ExecutionContext& other);

 private:
  ucontext_t _context;
};

}  // namespace yaclib
