#include <yaclib/algo/wait_group.hpp>

namespace yaclib {

WaitGroup::WaitGroup() {
  _callback.IncRef();
}

template void WaitGroup::Add<void>(Future<void>&);

void WaitGroup::Add(Future<void>&& f) {
  Add(f);
  std::move(f).Detach();
}

void WaitGroup::Wait() {
  _callback.DecRef();
  std::unique_lock guard{_callback.m};
  _callback.Wait(guard);
  _callback.is_ready = false;
  _callback.IncRef();
}

}  // namespace yaclib
