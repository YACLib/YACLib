#pragma once

#include "../stack_builder.h"
#include "yaclib/coroutines/context/machine_context.h"
#include "yaclib/coroutines/context/stack_view.h"

// https://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64/
static void MachineContextTrampoline(void*, void*, void*, void*, void*, void*, void* arg1, void* arg2);

void* SetupStack(StackView stack, Trampoline trampoline, void* arg);