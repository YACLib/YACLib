#include "yaclib/coroutines/context/execution_context.h"
void ExecutionContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _machine_context.Setup(stack, trampoline, arg);
}
