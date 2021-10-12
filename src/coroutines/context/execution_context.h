#pragma once

#include "machine_context.h"
#include "stack.h"

#include <algorithm>
class ExecutionContext {
 public:
  ExecutionContext() = default;

  void Setup(StackView stack, Trampoline trampoline, void* arg);

  MachineContext& GetMachineContext() {
    return _machine_context;
  };

  void SwitchTo(ExecutionContext& other) {
    _machine_context.SwitchTo(other.GetMachineContext());
  }

 private:
  MachineContext _machine_context;
};
