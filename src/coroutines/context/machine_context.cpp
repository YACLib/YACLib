#include <yaclib/coroutines/context/machine_context.hpp>

extern "C" void __yclib_switch_context(void* from_context, void* to_context);

void MachineContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  SetupStack(stack, trampoline, arg, _context);
}

void MachineContext::SwitchTo(MachineContext& target) {
  __yclib_switch_context(&_context, &target._context);
}
