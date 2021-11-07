#pragma once

#include <yaclib/fiber/detail/execution_context.hpp>
#include <yaclib/fiber/stack_view.hpp>
#include <yaclib/util/func.hpp>

#include <utility>

namespace yaclib {

using Routine = yaclib::util::IFuncPtr;

/**
 * Base coroutine class
 */
class CoroutineBase {
 public:
  CoroutineBase(const StackView& stack_view, Routine routine);

  void Resume();

  void Yield();

  [[nodiscard]] bool IsCompleted() const;

 private:
  [[noreturn]] static void Trampoline(void* arg);

  void Complete();

  ExecutionContext _context{};
  ExecutionContext _caller_context{};
  Routine _routine;
  // TODO(myannyax) union _completed and _exception
  bool _completed = false;
  std::exception_ptr _exception;
};

}  // namespace yaclib
