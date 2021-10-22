#pragma once
#include <yaclib/coroutines/context/stack.hpp>
#include <yaclib/coroutines/context/stack_allocator.hpp>
#include <yaclib/coroutines/coroutine.hpp>

namespace yaclib::coroutines {

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

}  // namespace yaclib::coroutines
