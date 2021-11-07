#pragma once

#include <yaclib/fiber/stack_view.hpp>

namespace yaclib {

using Trampoline = void (*)(void* arg);

// TODO(myannyax) check how much saving on stack is better + if depending on needed context size
inline constexpr size_t kAsmContextSize = 8;

void SetupStack(StackView stack, Trampoline trampoline, void* arg, void** context);

}  // namespace yaclib
