#pragma once

#include "machine_context.hpp"
#include "stack_view.hpp"

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
