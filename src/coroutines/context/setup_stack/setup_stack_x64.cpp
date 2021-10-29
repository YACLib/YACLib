#include "setup_stack_x64.hpp"

#include <cstdint>

namespace yaclib {

static void MachineContextTrampoline(void* arg1, void* arg2) {
  auto trampoline = (Trampoline)arg1;
  trampoline(arg2);
}

static const int kAlignment = 16;

void SetupStack(StackView stack, Trampoline trampoline, void* arg, YaclibFiberMachineContext& context) {
  size_t shift = (size_t)(stack.Back() - (sizeof(uintptr_t))) % kAlignment;
  char* top = stack.Back() - shift;

  context.RSP = top;
  context.RDI = (void*)trampoline;
  context.RSI = arg;

  context.RIP = (void*)MachineContextTrampoline;
}

}  // namespace yaclib
