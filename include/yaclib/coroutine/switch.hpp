#pragma once

#include <yaclib/coroutine/detail/coroutine.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/task.hpp>

#include <iostream>  // for debug

#include <yaclib/fault/thread.hpp>


namespace yaclib::detail {
// template <typename V, typename E>
class SwitchAwaiter : public ITask {
 public:
  SwitchAwaiter(yaclib::IExecutorPtr e) : _executor(std::move(e)) {
  }

  void IncRef() noexcept final {
    counter.fetch_add(1, std::memory_order_seq_cst);  // todo change memorder
    std::cout << "IncRef\n";
  }
  virtual void DecRef() noexcept override {
    std::cout << "DecRef\n";
    counter.fetch_sub(1, std::memory_order_seq_cst);  // todo change memorder
  }

  void Call() noexcept final {
    std::cout << "Call\n";
    using namespace std::chrono_literals;
    if (_handle && !_handle.done()) {
      std::cout << "resumed\n";
      _handle.resume();
      std::cout << "after resumed in call\n";
    }
    // _handle.promise().DecRef();
  }

  void Cancel() noexcept final {
    std::cout << "Cancel\n";

    // do smth
  }

  bool await_ready() {
    return false;
  }

  void await_resume() {
  }

  bool await_suspend(yaclib_std::coroutine_handle<> handle) {
    _handle = handle;
    // handle.promise().IncRef();
    std::cout << "b4 submit: " << std::endl;
    bool submitted = _executor->Submit(*this);
    std::cout << "submitted: " << submitted << std::endl;

    return submitted;
  }

  ~SwitchAwaiter() {
    std::cout << "Switch dtor" << std::endl;
  }

 private:
  yaclib::IExecutorPtr _executor = nullptr;
  yaclib_std::coroutine_handle<> _handle;
  yaclib_std::atomic_int counter = 0;  // todo atomic
};
}  // namespace yaclib::detail