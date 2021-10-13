#include <yaclib/coroutines/standalone_coroutine.hpp>
#include <yaclib/util/defer.hpp>

static thread_local StandaloneCoroutine* current = nullptr;

void StandaloneCoroutine::operator()() {
  Resume();
}
void StandaloneCoroutine::Resume() {
  StandaloneCoroutine* prev = current;
  current = this;

  yaclib::util::detail::DeferAction rollback([prev]() {
    current = prev;
  });

  _impl.Resume();
}
void StandaloneCoroutine::Yield() {
  current->_impl.Yield();
}

bool StandaloneCoroutine::IsCompleted() const {
  return _impl.IsCompleted();
}
