#pragma once

#include <yaclib/fiber/detail/setup_stack_x64.hpp>
#include <yaclib/fiber/stack_view.hpp>

#include <algorithm>

namespace yaclib {

class ExecutionContext {
 public:
  void Setup(StackView stack, Trampoline trampoline, void* arg);

  void SwitchTo(ExecutionContext& other);

 private:
  void* _context[kAsmContextSize];
};

}  // namespace yaclib
