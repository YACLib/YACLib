#pragma once
#include <yaclib/coroutines/context/stack_allocator.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

namespace yaclib {

// TODO some smart c++ stuff for zero cost abstraction
using Routine = yaclib::util::IFuncPtr;

class IStandaloneCoroutine : public util::IRef {
 public:
  virtual void operator()() = 0;

  virtual void Resume() = 0;

  static void Yield();

  [[nodiscard]] virtual bool IsCompleted() const = 0;

  virtual ~IStandaloneCoroutine() = default;
};

using IStandaloneCoroutinePtr = util::Ptr<IStandaloneCoroutine>;

class IStandaloneCoroutineFactory : public util::IRef {
 public:
  virtual IStandaloneCoroutinePtr New(StackAllocator& allocator, Routine routine) = 0;

  virtual IStandaloneCoroutinePtr New(Routine routine) {
    return New(GetAllocator(), std::move(routine));
  }

  virtual StackAllocator& GetAllocator() = 0;

  virtual ~IStandaloneCoroutineFactory() = default;
};

using IStandaloneCoroutineFactoryPtr = util::Ptr<IStandaloneCoroutineFactory>;

IStandaloneCoroutineFactoryPtr MakeStandaloneCoroutineFactory(StackAllocator& allocator);

}  // namespace yaclib
