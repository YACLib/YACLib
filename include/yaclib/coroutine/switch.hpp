#pragma once

#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>
#include <yaclib/fault/thread.hpp>
#include <yaclib/util/result.hpp>

#include <iostream>  // for debug

namespace yaclib::detail {



// Maybe via Type eraseure ? (need 1 allocation, maybe via buffer_field[sizeof(CoroHandlerImpl)] if valid?)

// struct CoroHandlerInterface {
//   virtual void Inc() noexcept;
//   virtual bool SubEqual(std::size_t) noexcept;
//   virtual void SafeResume() noexcept;
// };

// template <typename V, typename E>
// class CoroHandlerImpl : public CoroHandlerInterface {
//  public:
//   CoroHandlerImpl(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) : _handle(handle) {
//   }

//   void Inc() noexcept final {
//     _handle.promise().IncRef();
//   }
//   bool SubEqual(std::size_t x) noexcept final {
//     return _handle.promise().SubEqual(x);
//   }
//   void SafeResume() final {
//     if (_handle && !_handle.done()) {
//       _handle.resume();
//     }
//   }

//  private:
//   yaclib_std::coroutine_handle<PromiseType<V, E>> _handle;
// };

template <typename V, typename E>
class SwitchAwaiter : public ITask {
 public:
  SwitchAwaiter(yaclib::IExecutorPtr e) : _executor(std::move(e)) {
  }

  void IncRef() noexcept final {
    counter.fetch_add(1, std::memory_order_seq_cst);  // todo change memorder
  }
  void DecRef() noexcept final {
    counter.fetch_sub(1, std::memory_order_seq_cst);  // todo change memorder
  }

  void Call() noexcept final {
    using namespace std::chrono_literals;
    if (_handle && !_handle.done()) {
      _handle.resume();
    }
    _handle.promise().DecRef();
  }

  void Cancel() noexcept final {
    _handle.promise().Set(StopTag{});
    if (_handle.promise().SubEqual(1)) {
      assert(_handle);
      assert(_handle.done());
      _handle.destroy();
    }
  }

  bool await_ready() {
    return false;
  }

  void await_resume() {
  }

  bool await_suspend(yaclib_std::coroutine_handle<PromiseType<V, E>> handle) {
    _handle = handle;
    handle.promise().IncRef();

    return _executor->Submit(*this);
  }

 private:
  yaclib::IExecutorPtr _executor = nullptr;
  yaclib_std::coroutine_handle<PromiseType<V, E>> _handle;
  yaclib_std::atomic_int counter = 0;  // todo atomic
};
}  // namespace yaclib::detail