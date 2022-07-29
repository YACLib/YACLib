#include <yaclib/async/future.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/oneshot_event.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/type_traits.hpp>

namespace yaclib {

class AwaitGroup final {
 public:
  AwaitGroup() = default;
  void Add(std::size_t count = 1) noexcept {
    _await_core.IncRef(count);
  }
  void Done(std::size_t count = 1) noexcept {
    _await_core.DecRef(count);
  }
  template <bool NeedAdd = true, typename... V, typename... E>
  YACLIB_INLINE void Add(FutureBase<V, E>&... futures);

  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE std::enable_if_t<!is_future_base_v<It>, void> Add(It begin, It end);

  template <bool NeedAdd = true, typename It>
  YACLIB_INLINE void Add(It begin, std::size_t count);

  auto Await(IExecutor& exec = CurrentThreadPool()) {
    return _await_core.Await(exec);
  }

  void Wait() {
    ::yaclib::Wait(_await_core);
  }

  auto operator co_await() {
    YACLIB_WARN(true, "Better use AwaitGroup::Await(executor)");
    return Await();
  }

  void Reset() noexcept {
    _await_core.Reset();
  }

 private:
  template <bool NeedAdd, typename... Cores>
  void AddCore(Cores&... cores);

  template <bool NeedAdd, typename Range>
  void AddRange(const Range& range, std::size_t count);

  struct OneShotEventShooter final {
    static void Delete(OneShotEvent& event) {
      event.Set();
    }
  };

  detail::AtomicCounter<OneShotEvent, OneShotEventShooter> _await_core{0};
};

YACLIB_INLINE void Wait(AwaitGroup&);

}  // namespace yaclib

#include <yaclib/coroutine/detail/await_group_impl.hpp>
