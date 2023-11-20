#pragma once

#include <yaclib/config.hpp>
#include <yaclib/fault/detail/fiber/bidirectional_intrusive_list.hpp>
#include <yaclib/fault/detail/fiber/default_allocator.hpp>
#include <yaclib/fault/detail/fiber/execution_context.hpp>
#include <yaclib/fault/detail/fiber/stack.hpp>
#include <yaclib/fault/detail/fiber/stack_allocator.hpp>
#include <yaclib/fault/detail/fiber/wakeup_helper.hpp>

#include <exception>
#include <unordered_map>

namespace yaclib::detail::fiber {

class BiNodeScheduler : public Node {};

class BiNodeWaitQueue : public Node {};

enum FiberState {
  Running,
  Suspended,
  Waiting,  // TODO(MBkkt) remove, looks useless
  Completed,
};

class FiberBase : public BiNodeScheduler, public BiNodeWaitQueue {
 public:
  using Id = std::uint64_t;

  FiberBase();

  void SetJoiningFiber(FiberBase* joining_fiber) noexcept;

  [[nodiscard]] Id GetId() const noexcept;

  void Resume();

  void Suspend();

  FiberState GetState() noexcept;

  void SetState(FiberState state) noexcept;

  void SetThreadDead() noexcept;

  [[nodiscard]] bool IsThreadAlive() const noexcept;

  void* GetTLS(std::uint64_t id, std::unordered_map<std::uint64_t, void*>& defaults);

  void SetTLS(std::uint64_t id, void* value);

  static IStackAllocator& GetAllocator() noexcept;

  virtual ~FiberBase() = default;

 protected:
  void Start();
  void Exit();

  ExecutionContext _context{};
  Stack _stack;
  std::exception_ptr _exception;

 private:
  ExecutionContext _caller_context{};
  std::unordered_map<std::uint64_t, void*> _tls;
  FiberBase* _joining_fiber = nullptr;
  Id _id;
  FiberState _state = Suspended;
  bool _thread_alive = true;
};

}  // namespace yaclib::detail::fiber
