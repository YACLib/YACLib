#include "asm/switch_context_x86_64.hpp"

#include <yaclib/fiber/detail/setup_stack_x64.hpp>

extern "C" void yaclib_trampoline();

namespace yaclib {

static inline constexpr int kAlignment = 16;

void SetupStack(StackView stack, Trampoline trampoline, void* arg, void** context) {
  stack.Align(kAlignment);

  stack.Push((void*)trampoline);
  stack.Push(arg);

  context[YACLIB_RSP_INDEX] = stack.Back();
  context[YACLIB_RIP_INDEX] = (void*)(yaclib_trampoline);
}

}  // namespace yaclib
