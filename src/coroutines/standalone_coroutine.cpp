#include <yaclib/coroutines/standalone_coroutine.hpp>

static thread_local StandaloneCoroutine* current = nullptr;

void StandaloneCoroutine::operator()() {
  Resume();
}
void StandaloneCoroutine::Resume() {
  StandaloneCoroutine* prev = current;
  current = this;

  _impl.Resume();
  current = prev;
}
void StandaloneCoroutine::Yield() {
  current->_impl.Yield();
}

bool StandaloneCoroutine::IsCompleted() const {
  return _impl.IsCompleted();
}
