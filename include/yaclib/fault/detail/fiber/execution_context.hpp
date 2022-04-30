#pragma once

#include <yaclib/fault/detail/fiber/stack_allocator.hpp>

#include <algorithm>

#include <ucontext.h>

namespace yaclib::detail {

using Trampoline = void (*)(void* arg);

class ExecutionContext {
 public:
  void Setup(Allocation stack, Trampoline trampoline, void* arg);

  void SwitchTo(ExecutionContext& other);

 private:
  ucontext_t _context;
};

}  // namespace yaclib::detail
