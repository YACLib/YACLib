#include "standalone_coroutine_factory.hpp"

#include "standalone_coroutine_impl.hpp"

namespace yaclib {

IStandaloneCoroutinePtr StandaloneCoroutineFactory::New(StackAllocator& allocator,
                                                        Routine routine) {
  return new util::Counter<StandaloneCoroutineImpl>{allocator, std::move(routine)};
}

IStandaloneCoroutinePtr StandaloneCoroutineFactory::New(Routine routine) {
  return IStandaloneCoroutineFactory::New(routine);
}

StackAllocator& StandaloneCoroutineFactory::GetAllocator() {
  return _allocator;
}

void StandaloneCoroutineFactory::IncRef() noexcept {
}

void StandaloneCoroutineFactory::DecRef() noexcept {
}

IStandaloneCoroutineFactoryPtr MakeStandaloneCoroutineFactory(StackAllocator& allocator) {
  return new util::Counter<StandaloneCoroutineFactory>{allocator};
}

}  // namespace yaclib
