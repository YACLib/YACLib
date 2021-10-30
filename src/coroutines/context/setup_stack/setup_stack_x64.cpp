#include "setup_stack_x64.hpp"

#include <cstdint>
#include <cstdio>

extern "C" void yaclib_trampoline();

namespace yaclib {

static const int kAlignment = 16;

void SetupStack(StackView stack, Trampoline trampoline, void* arg, AsmContext& context) {
  size_t shift = (size_t)(stack.Back() - (sizeof(uintptr_t))) % kAlignment;
  char* top = stack.Back() - shift;

  top -= sizeof(void*);
  *(void**)top = (void*)(trampoline);
  top -= sizeof(void*);
  *(void**)top = arg;

  context.RSP = top;
  context.RIP = (void*)(yaclib_trampoline);
}

}  // namespace yaclib
