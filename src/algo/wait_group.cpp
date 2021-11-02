#include <yaclib/algo/wait_group.hpp>

namespace yaclib {

WaitGroup::WaitGroup() {
  _callback.IncRef();
}

void WaitGroup::Wait() {
  _callback.DecRef();
  std::unique_lock guard{_callback.m};
  _callback.Wait(guard);
  _callback.is_ready = false;
  _callback.IncRef();
}

}  // namespace yaclib
