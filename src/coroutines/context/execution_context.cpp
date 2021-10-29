#include "execution_context.hpp"

namespace yaclib {

void ExecutionContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _machine_context.Setup(stack, trampoline, arg);
}

}  // namespace yaclib
