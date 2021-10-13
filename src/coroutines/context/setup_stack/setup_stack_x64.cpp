#pragma once

#include "setup_stack.h"

struct StackSavedMachineContext {
  // Layout of the StackSavedMachineContext matches the layout of the stack
  // in machine_context.S at the 'Switch stacks' comment

  // Callee-saved registers
  // Saved manually in SwitchMachineContext
  void* rbp;
  void* rbx;

  void* r12;
  void* r13;
  void* r14;
  void* r15;

  // Saved automatically by 'call' instruction
  void* rip;
};

static void MachineContextTrampoline(void*, void*, void*, void*, void*, void*, void* arg1, void* arg2) {
  auto trampoline = (Trampoline)arg1;
  trampoline(arg2);
}

void* SetupStack(StackView stack, Trampoline trampoline, void* arg) {
  StackBuilder builder(stack.Back());

  builder.Allocate(sizeof(uintptr_t) * 3);

  builder.AlignNextPush(16);

  ArgumentsListBuilder args(builder.Top());
  args.Push((void*)trampoline);
  args.Push(arg);

  builder.Allocate(sizeof(StackSavedMachineContext));

  auto* stack_saved_context = (StackSavedMachineContext*)builder.Top();
  stack_saved_context->rip = (void*)MachineContextTrampoline;

  return stack_saved_context;
}