#pragma once
#include "coroutine.h"
#include "yaclib/coroutines/context/stack.h"

#include <yaclib/coroutines/context/stack_allocator.h>

class StandaloneCoroutine {
 public:
  StandaloneCoroutine(StackAllocator& allocator, Routine routine)
      : _stack(allocator.Allocate(), allocator), _impl(_stack.View(), std::move(routine)) {
  }
  void operator()();
  void Resume();
  static void Yield();
  bool IsCompleted() const;

 private:
  Stack _stack;
  Coroutine _impl;
};
