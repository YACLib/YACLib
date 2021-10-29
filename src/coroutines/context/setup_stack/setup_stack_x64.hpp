#pragma once

#include <yaclib/coroutines/context/stack_view.hpp>

namespace yaclib {

using Trampoline = void (*)(void* arg);

struct YaclibFiberMachineContext {
  [[maybe_unused]] void* RBX;
  [[maybe_unused]] void* RBP;
  [[maybe_unused]] void* R12;
  [[maybe_unused]] void* R13;
  [[maybe_unused]] void* R14;
  [[maybe_unused]] void* R15;

  [[maybe_unused]] void* RDI;  // from
  [[maybe_unused]] void* RSI;  // to

  [[maybe_unused]] void* RSP;
  [[maybe_unused]] void* RIP;
};

void SetupStack(StackView stack, Trampoline trampoline, void* arg, YaclibFiberMachineContext& context);

}  // namespace yaclib
