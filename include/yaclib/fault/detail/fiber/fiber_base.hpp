#pragma once

#include <yaclib/fault/detail/fiber/bidirectional_intrusive_list.hpp>
#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/execution_context.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/fault/detail/fiber/wakeup_helper.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <unordered_map>

namespace yaclib::detail::fiber {

class BiNodeScheduler : public BiNode {};

class BiNodeWaitQueue : public BiNode {};

enum FiberState {
  Running,
  Suspended,
  Completed,
};

class FiberBase : public BiNodeScheduler, public BiNodeWaitQueue {
 public:
  using Id = uint64_t;

  FiberBase();

  void SetJoiningFiber(FiberBase* joining_fiber);

  [[nodiscard]] Id GetId() const;

  void Resume();

  void Yield();

  FiberState GetState();

  void SetThreadlikeInstanceDead();

  [[nodiscard]] bool IsThreadlikeInstanceAlive() const;

  void* GetTls(uint64_t name, void* _default);

  void SetTls(uint64_t name, void* value);

  static IStackAllocator& GetAllocator();

  virtual ~FiberBase() = default;

 protected:
  void Complete();

  Stack _stack;
  ExecutionContext _context{};
  std::exception_ptr _exception;

 private:
  static DefaultAllocator allocator;
  ExecutionContext _caller_context{};
  FiberBase* _joining_fiber{nullptr};
  Id _id;
  FiberState _state{Suspended};
  bool _threadlike_instance_alive{true};
  std::unordered_map<uint64_t, void*> _tls;
};

}  // namespace yaclib::detail::fiber
