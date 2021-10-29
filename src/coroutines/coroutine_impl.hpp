#pragma once
#include "coroutines/context/execution_context.hpp"

#include <yaclib/coroutines/context/stack_view.hpp>
#include <yaclib/coroutines/standalone_coroutine.hpp>

#include <utility>

namespace yaclib {
/***
 * base coroutine class
 */
class CoroutineImpl {
 public:
  CoroutineImpl(const StackView& stack_view, Routine routine) : _routine(std::move(routine)) {
    _context.Setup(stack_view, Trampoline, this);
  }

  void operator()();

  void Resume();

  void Yield();

  [[nodiscard]] bool IsCompleted() const;

 private:
  [[noreturn]] static void Trampoline(void* arg);

  void Complete();

  ExecutionContext _context{};
  ExecutionContext _caller_context{};
  Routine _routine;
  bool _completed = false;
  std::exception_ptr _exception;
};

}  // namespace yaclib
