#pragma once
#include "coroutine_impl.hpp"

#include <yaclib/coroutines/context/default_allocator.hpp>
#include <yaclib/coroutines/context/stack.hpp>
#include <yaclib/coroutines/context/stack_allocator.hpp>
#include <yaclib/coroutines/standalone_coroutine.hpp>

namespace yaclib::coroutines {

class StandaloneCoroutineImpl : public IStandaloneCoroutine {
 public:
  StandaloneCoroutineImpl(StackAllocator& allocator, Routine routine);

  void operator()() override;

  void Resume() override;

  bool IsCompleted() const override;

  ~StandaloneCoroutineImpl() override = default;

  static void Yield();

  void IncRef() noexcept override;

  void DecRef() noexcept override;

 private:
  Stack _stack;
  CoroutineImpl _impl;
};

}  // namespace yaclib::coroutines
