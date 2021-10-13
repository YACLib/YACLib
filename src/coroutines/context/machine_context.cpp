#include "yaclib/coroutines/context/machine_context.h"

#include "setup_stack/setup_stack.h"

extern "C" void SwitchMachineContext(void* from_rsp, void* to_rsp);

void MachineContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _rsp = SetupStack(stack, trampoline, arg);
}

void MachineContext::SwitchTo(MachineContext& target) {
  SwitchMachineContext(&_rsp, &target._rsp);
}