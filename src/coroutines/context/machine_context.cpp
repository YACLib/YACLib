#include "machine_context.h"

extern "C" void SwitchMachineContext(void* from_rsp, void* to_rsp);

void MachineContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  _rsp;
  // TODO
}
void MachineContext::SwitchTo(MachineContext& target) {
  // TODO
}
void* MachineContext::SetupStack(StackView stack, Trampoline trampoline, void* arg) {
  // TODO
}