#pragma once

#include <yaclib/fault/detail/fiber/execution_context.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/util/detail/shared_func.hpp>

#include <utility>

namespace yaclib::detail {

using Routine = yaclib::IFuncPtr;

/**
 * Base coroutine class
 */
class CoroutineBase {
 public:
  CoroutineBase(Allocation stack_view, Routine routine);

  void Resume();

  void Yield();

  [[nodiscard]] bool IsCompleted() const;

 private:
  [[noreturn]] static void Trampoline(void* arg);

  void Complete();

  ExecutionContext _context{};
  ExecutionContext _caller_context{};
  Routine _routine;
  // TODO(myannyax): union _completed and _exception
  bool _completed = false;
  std::exception_ptr _exception;
};

}  // namespace yaclib::detail
