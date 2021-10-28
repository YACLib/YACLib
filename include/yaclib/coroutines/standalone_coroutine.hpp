#pragma once
#include <yaclib/coroutines/context/stack.hpp>
#include <yaclib/coroutines/context/stack_allocator.hpp>
#include <yaclib/coroutines/coroutine.hpp>
#include "yaclib/coroutines/context/default_allocator.hpp"

namespace yaclib::coroutines {

class StandaloneCoroutine {
 public:
  StandaloneCoroutine(StackAllocator& allocator, Routine routine)
      : _stack(allocator.Allocate(), allocator), _impl(_stack.View(), std::move(routine)) {
  }

  explicit StandaloneCoroutine(Routine routine) : StandaloneCoroutine(yaclib::coroutines::default_allocator_instance, std::move(routine)){}

  void operator()();

  void Resume();

  static void Yield();

  [[nodiscard]] bool IsCompleted() const;

 private:
  Stack _stack;
  Coroutine _impl;
};

}  // namespace yaclib::coroutines
