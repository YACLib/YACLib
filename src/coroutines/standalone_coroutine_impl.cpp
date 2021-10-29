#include "standalone_coroutine_impl.hpp"

namespace yaclib {

static thread_local StandaloneCoroutineImpl* current = nullptr;

StandaloneCoroutineImpl::StandaloneCoroutineImpl(StackAllocator& allocator, Routine routine)
    : _stack(allocator.Allocate(), allocator), _impl(_stack.View(), std::move(routine)) {
}

void StandaloneCoroutineImpl::operator()() {
  Resume();
}
void StandaloneCoroutineImpl::Resume() {
  StandaloneCoroutineImpl* prev = current;
  current = this;

  _impl.Resume();
  current = prev;
}

void StandaloneCoroutineImpl::Yield() {
  current->_impl.Yield();
}

bool StandaloneCoroutineImpl::IsCompleted() const {
  return _impl.IsCompleted();
}

void StandaloneCoroutineImpl::IncRef() noexcept {
}

void StandaloneCoroutineImpl::DecRef() noexcept {
}

}  // namespace yaclib
