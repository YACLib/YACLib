#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/fault/atomic.hpp>

namespace yaclib {

template void WaitGroup::Add<void, StopError>(Future<void, StopError>&);

void WaitGroup::Wait() {
  if (_callback.SubEqual(1)) {  // TODO(MBkkt) rel + fence(acquire) instead of acq_rel
    return _callback.count.store(1, std::memory_order_relaxed);
  }
  std::unique_lock lock{_callback.m};
  _callback.Wait(lock);
  _callback.is_ready = false;
  _callback.count.store(1, std::memory_order_relaxed);
}

void WaitGroup::Add(detail::BaseCore& core) {
  if (!core.Empty()) {
    return;
  }
  _callback.IncRef();
  if (!core.SetWait(_callback)) {
    _callback.count.fetch_sub(1, std::memory_order_relaxed);
  }
}

}  // namespace yaclib
