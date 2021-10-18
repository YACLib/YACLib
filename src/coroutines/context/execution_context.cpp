#include <yaclib/coroutines/context/execution_context.hpp>

void ExecutionContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _machine_context.Setup(stack, trampoline, arg);
}
