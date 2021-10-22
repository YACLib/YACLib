#include <yaclib/coroutines/context/execution_context.hpp>

namespace yaclib::coroutines {

void ExecutionContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _machine_context.Setup(stack, trampoline, arg);
}

}  // namespace yaclib::coroutines
