#include "machine_context.hpp"

extern "C" void __yaclib_switch_context(void* from_context, void* to_context);

namespace yaclib {

void MachineContext::Setup(StackView stack, Trampoline trampoline, void* arg) {
  SetupStack(stack, trampoline, arg, _context);
}

void MachineContext::SwitchTo(MachineContext& target) {
  __yaclib_switch_context(&_context, &target._context);
}

}  // namespace yaclib
