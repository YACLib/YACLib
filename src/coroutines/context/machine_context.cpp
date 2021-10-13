#include "yaclib/coroutines/context/machine_context.hpp"

#include "coroutines/context/setup_stack/setup_stack.hpp"

extern "C" void SwitchMachineContext(void* from_rsp, void* to_rsp);

void MachineContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _rsp = SetupStack(stack, trampoline, arg);
}

void MachineContext::SwitchTo(MachineContext& target) {
  SwitchMachineContext(&_rsp, &target._rsp);
}
