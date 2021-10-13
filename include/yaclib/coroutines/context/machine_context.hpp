#pragma once
#include "stack_view.hpp"

using Trampoline = void (*)(void* arg);

/***
 * registers and stack switch
 */
class MachineContext {
 public:
  MachineContext() = default;

  void Setup(StackView stack, Trampoline trampoline, void* arg);

  void SwitchTo(MachineContext& target);

 private:
  void* _rsp;
};
