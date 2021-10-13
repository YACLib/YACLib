#pragma once

#include "../stack_builder.hpp"
#include <yaclib/coroutines/context/machine_context.hpp>
#include <yaclib/coroutines/context/stack_view.hpp>

// https://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64/
static void MachineContextTrampoline(void*, void*, void*, void*, void*, void*, void* arg1, void* arg2);

void* SetupStack(StackView stack, Trampoline trampoline, void* arg);
