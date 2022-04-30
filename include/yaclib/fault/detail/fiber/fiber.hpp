#pragma once

#include <yaclib/fault/detail/fiber/coroutine_base.hpp>
#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>

namespace yaclib::detail {

enum FiberState {
  Running,
  Suspended,
};

class Fiber : public IRef {
 public:
  using Id = unsigned long;

  friend class FiberThreadlike;
  friend class Scheduler;

  Fiber(Fiber&& other) noexcept = default;

  Fiber& operator=(Fiber&& other) noexcept = default;

  Fiber(IStackAllocator& allocator, Routine routine);

  explicit Fiber(Routine routine);

  Id GetId();

  ~Fiber() override = default;

  void IncRef() noexcept override;
  void DecRef() noexcept override;

 private:
  void Resume();

  void Yield();

  static Id _next_id;
  Stack _stack;
  Id _id;
  CoroutineBase _impl;
  FiberState _state;
};

}  // namespace yaclib::detail
