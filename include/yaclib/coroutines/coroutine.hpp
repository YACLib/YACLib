#pragma once
#include <yaclib/coroutines/context/execution_context.hpp>
#include <yaclib/coroutines/context/stack_view.hpp>
#include <yaclib/util/func.hpp>

#include <utility>

namespace yaclib::coroutines {

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

}  // namespace yaclib::coroutines
