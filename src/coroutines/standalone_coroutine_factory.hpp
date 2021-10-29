#pragma once
#include <yaclib/coroutines/standalone_coroutine.hpp>

namespace yaclib::coroutines {

class StandaloneCoroutineFactory : public IStandaloneCoroutineFactory {
 public:
  explicit StandaloneCoroutineFactory(StackAllocator& allocator) : _allocator(allocator){};

  IStandaloneCoroutinePtr New(StackAllocator& allocator, Routine routine) override;

  IStandaloneCoroutinePtr New(Routine routine) override;

  StackAllocator& GetAllocator() override;

  void IncRef() noexcept override;

  void DecRef() noexcept override;

  ~StandaloneCoroutineFactory() override = default;

 private:
  StackAllocator& _allocator;
};

}  // namespace yaclib::coroutines
