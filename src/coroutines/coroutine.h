#pragma once
#include "coroutines/context/execution_context.h"
#include "coroutines/context/stack.h"
#include "coroutines/context/stack_allocator.h"
#include "yaclib/util/func.hpp"

#include <utility>

// TODO some smart c++ stuff for zero cost abstraction
using Routine = yaclib::util::IFuncPtr;

/***
 * base coroutine class
 */
class Coroutine {
 public:
  Coroutine(const StackView& stack_view, Routine routine) : _routine(std::move(routine)) {
    _context.Setup(stack_view, Trampoline, this);
  }
  void operator()();
  void Resume();
  void Yield();
  bool IsCompleted() const;

 private:
  [[noreturn]] static void Trampoline(void* arg);
  void Complete();

  ExecutionContext _context;
  ExecutionContext _caller_context;
  Routine _routine;
  bool _completed = false;
  std::exception_ptr _exception;
};