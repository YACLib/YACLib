#pragma once

#include <yaclib/coroutines/context/stack_view.hpp>

using Trampoline = void (*)(void* arg);

struct YaclibFiberMachineContext {
  void* RBX;
  void* RBP;
  void* R12;
  void* R13;
  void* R14;
  void* R15;

  void* RDI; //from
  void* RSI; //to

  void* RSP;
  void* RIP;
};

void SetupStack(StackView stack, Trampoline trampoline, void* arg,
                                      YaclibFiberMachineContext& context);
