#pragma once

#include <yaclib/fault/detail/fiber/bidirectional_intrusive_list.hpp>
#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/execution_context.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/fault/detail/fiber/wakeup_helper.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <exception>
#include <unordered_map>

namespace yaclib::detail::fiber {

class BiNodeScheduler : public Node {};

class BiNodeWaitQueue : public Node {};

enum FiberState {
  Running,
  Suspended,
  Waiting,
  Completed,
};

class FiberBase : public BiNodeScheduler, public BiNodeWaitQueue {
 public:
  using Id = uint64_t;

  FiberBase();

  void SetJoiningFiber(FiberBase* joining_fiber) noexcept;

  [[nodiscard]] Id GetId() const noexcept;

  void Resume();

  void Yield();

  FiberState GetState() noexcept;

  void SetState(FiberState state) noexcept;

  void SetThreadlikeInstanceDead() noexcept;

  [[nodiscard]] bool IsThreadlikeInstanceAlive() const noexcept;

  void GetTls(uint64_t name, void** _default);

  void SetTls(uint64_t name, void* value);

  static IStackAllocator& GetAllocator() noexcept;

  virtual ~FiberBase() = default;

 protected:
  void Complete();

  ExecutionContext _context{};
  Stack _stack;
  std::exception_ptr _exception;

 private:
  static DefaultAllocator sAllocator;
  ExecutionContext _caller_context{};
  std::unordered_map<uint64_t, void*> _tls;
  FiberBase* _joining_fiber{nullptr};
  Id _id;
  FiberState _state{Suspended};
  bool _threadlike_instance_alive{true};
};

}  // namespace yaclib::detail::fiber
