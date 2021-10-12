#pragma once
#include "stack.h"
#include "stack_view.h"

#include <cstdint>

using Trampoline = void (*)(void* arg);

/***
 * registers and stack switch
 */
class MachineContext {
 public:
  MachineContext() = default;

  void* GetRsp() {
    return _rsp;
  };

  void Setup(StackView stack, Trampoline trampoline, void* arg);

  void SwitchTo(MachineContext& target);

 private:
  void* SetupStack(StackView stack, Trampoline trampoline, void* arg);
  void* _rsp;
};
