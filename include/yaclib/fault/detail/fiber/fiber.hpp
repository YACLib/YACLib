#pragma once

#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/execution_context.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/util/detail/shared_func.hpp>

namespace yaclib::detail {

using Routine = yaclib::IFuncPtr;

enum FiberState {
  Running,
  Suspended,
  Completed,
};

class Fiber : public IRef {
 public:
  using Id = uint64_t;

  Fiber(Fiber&& other) noexcept = default;

  Fiber& operator=(Fiber&& other) noexcept = default;

  Fiber(IStackAllocator& allocator, Routine routine);

  explicit Fiber(Routine routine);

  void SetCompleteCallback(Routine routine);

  [[nodiscard]] Id GetId() const;

  void Resume();

  void Yield();

  FiberState GetState();

  void IncRef() noexcept override;
  void DecRef() noexcept override;

 private:
  [[noreturn]] static void Trampoline(void* arg);

  void Complete();

  Stack _stack;
  ExecutionContext _context{};
  ExecutionContext _caller_context{};
  Routine _routine;
  Routine _complete_callback{nullptr};
  std::exception_ptr _exception;
  Id _id;
  FiberState _state{Suspended};
};

}  // namespace yaclib::detail
