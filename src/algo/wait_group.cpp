#include <yaclib/algo/wait_group.hpp>
#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/config.hpp>
#include <yaclib/fault/atomic.hpp>

namespace yaclib {

void WaitGroup::Wait() {
  if (_event.SubEqual(1)) {  // TODO(MBkkt) rel + fence(acquire) instead of acq_rel
    return _event.count.store(1, std::memory_order_relaxed);
  }
  auto token = _event.Make();
  _event.Wait(token);
  _event.Reset(token);
  _event.count.store(1, std::memory_order_relaxed);
}

void WaitGroup::Add(detail::BaseCore& core) {
  if (!core.Empty()) {
    return;
  }
  _event.IncRef();
  if (!core.SetWait(_event)) {
    _event.count.fetch_sub(1, std::memory_order_relaxed);
  }
}

}  // namespace yaclib
